import sys
import os
from sys import stdin
import csv
import numpy as np
import time
import threading

COLUMNS = 35
# this will hold the timestamp data
data = np.zeros((1,COLUMNS), dtype=int)
PENDING = -2

isComplete = False

# Current largest msg id (will increase over time)
largestMsgId = 0

labels = {
    "add2q_l": (1, 8),
    "MSInrdy": (2, 2),
    "MSIredy": (3, 3),
    "msgProc": (4, 9),
    "LLCnrdy": (6, 6),
    "LLCredy": (7, 7),
    "add_req": (5, 10),
    "add2q_u": (16, 11),
    "respond": (17, 12),
    "datNrdy": (18, 13),
    "dataRdy": (19, 14),
    "updaDat": (20, 15),
    "writeBk": (21, 22),
    "add2q_r": (23, 24),
    "hitActn": (25, 26),
    "replace": (27, 28),
    "wrCache": (29, 30),
    "termina": (31, 31),
    "L1WBsrc": (32, 32),
    "Miss?  ": (33, 34)
}

# Components
comps = {
    "mw_init":    0,
    "stable_L1":  1,
    "A_req":      2,
    "A_data":     3,
    "mw_LLC":     4,
    "stable_LLC": 5,
    "A_DRAM":     6,
    "p":          7,
    "A_resp":     8,
    "w_end_L1":   9,
    "w_end_LLC":  10,
    "total":      11
}

COMPONENTS = len(comps) + 1
wcet = np.empty((0, COMPONENTS), dtype=int)
sigIndices = {}

# Find the difference between different timestamps to find the delays.
def findCompDelays(line):
    delays = np.zeros(COMPONENTS, dtype=int)
    while not checkDone(line):
        print("ERROR")
        os._exit(2)
    # First, find the message's starting point
    start = line[labels["add2q_l"][0]] or \
            line[labels["add2q_r"][0]] or \
            line[labels["add2q_r"][1]]
    if (start == 0):
       print("Error: no start")
       os._exit(3)
    # Now, find its end point
    end = max (
        line[labels["termina"][0]],
        line[labels["wrCache"][0]],
        line[labels["wrCache"][1]]
    )
    # Now, find the individual component delays
    hasDep = False
    depLine = np.zeros(COLUMNS, dtype=int)
    if line[labels["L1WBsrc"][0]] != 0:
        depLine = getDepLine(line[0] - 1)
        hasDep = True
    
    # These msgProg_L1 and (add2q_l or add2q_r) should either both be zero
    # or nonzero, so we don't need to check
    msgRdy_L1 = 0
    msgRdy_L1 = \
        line[labels["msgProc"][0]] - \
        (line[labels["add2q_l"][0]] or line[labels["add2q_r"][0]])
    # mw_init does not have dependencies
    if (line[labels["MSIredy"][0]] != 0 and line[labels["MSInrdy"][0]] != 0):
        delays[comps["mw_init"]] = \
            (line[labels["MSIredy"][0]] - line[labels["MSInrdy"][0]])
    # stable_L1
    delays[comps["stable_L1"]] = \
        msgRdy_L1 - delays[comps["mw_init"]]
    # A_req
    if (line[labels["add2q_l"][1]] != 0 and line[labels["add_req"][0]] != 0):
        delays[comps["A_req"]] = \
            line[labels["add2q_l"][1]] - line[labels["add_req"][0]]
    elif hasDep == True:
        diff = depLine[labels["add2q_l"][1]] - line[labels["add_req"][0]]
        delays[comps["A_req"]] = diff \
            if (diff > 0 and line[labels["add_req"][0]] != 0) else 0
    #if (delays[comps["A_req"]] > 90):
    #    print(str(line[0])+": "+str(delays[comps["A_req"]]))
    # A_data only happens in case of LLC hit
    # It doesn't happen for dependencies, since the other L1 sends the data
    if (line[labels["datNrdy"][1]] != 0 and line[labels["dataRdy"][1]] != 0):
        delays[comps["A_data"]] = \
            line[labels["dataRdy"][1]] - line[labels["datNrdy"][1]]
    elif (line[labels["datNrdy"][1]] != 0 and line[labels["respond"][1]] != 0):
        print("WARNING: datardy dropped out")
        delays[comps["A_data"]] = \
            line[labels["respond"][1]] - line[labels["datNrdy"][1]]
    #print(str(line[0])+": "+str(delays[comps["A_data"]]))
    # msgRdy_LLC (both are nonzero or zero)
    msgRdy_LLC = 0
    msgRdy_LLC = \
        line[labels["msgProc"][1]] - line[labels["add2q_l"][1]]
    # mw_LLC 
    if (line[labels["LLCredy"][0]] != 0 and line[labels["LLCnrdy"][0]] != 0):
        delays[comps["mw_LLC"]] = \
            line[labels["LLCredy"][0]] - line[labels["LLCnrdy"][0]]
    # stable_LLC
    delays[comps["stable_LLC"]] = \
        msgRdy_LLC - delays[comps["mw_LLC"]]
    # A_DRAM
    #if hasDep == True:
    #    delays[comps["A_DRAM"]] = \
    #        depLine[labels["add2q_u"][1]] - depLine[labels["LLCredy"][0]]
    #else:
    delays[comps["A_DRAM"]] = \
        line[labels["add2q_u"][1]] - line[labels["LLCredy"][0]]
    # p 
    if hasDep == True:
        reqStart = max(
            depLine[labels["respond"][1]], 
            line[labels["msgProc"][0]] or line[labels["msgProc"][1]]
        )
        delays[comps["p"]] = line[labels["writeBk"][0]] - reqStart
    #print(str(line[0])+": "+str(depLine[labels["respond"][1]]))
    #print(str(line[0])+": "+str(line[labels["msgProc"][1]]))
    if (delays[comps["p"]] > 100):
        print(str(line[0])+": "+str(delays[comps["p"]]))
    # A_resp
    delays[comps["A_resp"]] = \
        line[labels["add2q_u"][0]] - \
            (
                line[labels["respond"][1]] or\
                line[labels["writeBk"][0]] or\
                line[labels["add2q_u"][0]] # if both are zero, cancel out
            )
    #if (delays[comps["A_resp"]] > 21):
    #    print(str(line[0])+": "+str(delays[comps["A_resp"]]))
    # W_end_L1
    delays[comps["w_end_L1"]] = \
        (line[labels["wrCache"][0]] - line[labels["updaDat"][0]]) \
        if (line[labels["wrCache"][0]] != -1) else 0
    # W_end_LLC
    delays[comps["w_end_LLC"]] = \
        (line[labels["wrCache"][1]] - line[labels["updaDat"][1]]) \
        if (line[labels["wrCache"][1]] != -1) else 0
    # total delay
    delays[comps["total"]] = end - start
    #print(str(line[0])+": "+str(delays))
    return delays

def checkDone(row):
    return True if \
        (
            row[labels["wrCache"][0]] != 0 and \
            row[labels["wrCache"][1]] != 0 and \
            row[labels["termina"][0]] != 0
        ) \
        and np.where(row == PENDING)[0].shape[0] == 0 \
        else False

def parseLine(line):
    global data, largestMsgId
    # Split the line into its data fields
    field = line.split(',')
    row = int(field[0]) - 1
    col = labels[field[2]][0 if int(field[3]) != 10 else 1]
    value = int(field[4])

    if (int(field[0]) > largestMsgId):
        largestMsgId = int(field[0])
        #print(largestMsgId)

    # Check array dimensions, expand if too small
    dim = list(data.shape)
    dim[0] = row - (dim[0] - 1)
    dim[1] = col - (dim[1] - 1)
    if (dim[0] > 0 or dim[1] > 0):
        data = np.pad(
            data, (
                (0, (dim[0] + 500) if dim[0] >= 0 else 0),
                (0, dim[1] if dim[1] >= 0 else 0)
            )
        )

    # Index the array and write to the target cell
    data[row][0] = row + 1
    # Do not update the line if it's already set to complete; this should
    # not happen.
    if checkDone(data[row]):
        # We only take the first "termina" value for the line
        if (field[2] == "termina"):
            return
        print(str(data[row][0])+": "+field[2]+", "+field[3]+", "+field[4])
        #os._exit(2)
    if (col == labels["wrCache"][0] or col == labels["wrCache"][1]):
        if (data[row][col] == -1 and value != -1):
            print("Error at id "+str(row+1)+"; value = "+str(value))
            os._exit(1)
    
    if (col != labels["MSInrdy"][0] 
        and col != labels["LLCnrdy"][0]
        and col != labels["add2q_l"][1]):
        #if not (data[row][col] != 0 and value == -2):
        #    # If a value is already filled, we cannot set it to pending 
        #    # (this avoids any ugly temporal dependencies)
        data[row][col] = value 
    # If MSInrdy or LLCnrdy, we only want to record the first instance of this 
    # happening, so only write if the entry is currently empty
    elif (data[row][col] == 0):
        data[row][col] = value


# This thread writes the data array line by line to the data file; if the line 
# is not complete (i.e. the l1 respond field is empty) then it waits until it 
# is. Then it goes to the next line, etc.
def csvWrite():
    global isComplete
    print('hello')
    # open a new csv file
    with open("data.csv", 'w', newline='') as csvfile:
        writer = csv.writer(csvfile)
        # loop through each row of data
        i = 0
        #while i < data.shape[0]:
        while not (i >= largestMsgId and isComplete == True):
            if i >= data.shape[0]:
                time.sleep(0.1)
                continue
            #print(data[i])
            # wait until row is complete
            while not checkDone(data[i]):
                #print(data[i])
                #print("waiting...")
                time.sleep(0.1)
            #print("good...")
            # if row is complete, append to csv
            line = np.copy(data[i])
            line[labels["Miss?  "][1]] = getDepData(i, labels["Miss?  "][1])
            writer.writerow(line)
            i += 1
    print('csv done')
    return

# Loop through the data matrix, waiting for the next line to complete.
# When it has completed, see if its signature is already recorded.
# If not, add its signature to the matrix.
def sigGenMatrix():
    global isComplete, COLUMNS
    sigs = np.empty((0,COLUMNS), dtype=int)
    with open("sigs.csv", "w", newline='') as csvfile:
        writer = csv.writer(csvfile)
        i = 0
        while not (i >= largestMsgId and isComplete == True):
            if i >= data.shape[0]:
                time.sleep(0.1)
                continue
            while not checkDone(data[i]):
                time.sleep(0.1)
            line = np.copy(data[i])
            line[labels["Miss?  "][1]] = getDepData(i, labels["Miss?  "][1])
            # Find signature
            normLine = np.where(line > 0, 1, line)
            # If not currently in signature matrix, add it
            if not np.any(np.all(sigs[:,1:] == normLine[1:], axis=1)):
                # Restore the msg id in the signature
                normLine[0] = data[i][0]
                # Find the index of the new signature
                index = np.shape(sigs)[0]
                # Record it in the signature index dictionary
                key = tuple(normLine[1:])
                sigIndices[key] = index
                # Add new signature to signature matrix
                sigs = np.append(sigs, [normLine], axis=0)
                #print(normLine) 
            i += 1
        np.savetxt("sigs.csv", sigs, delimiter=",", fmt="%d")
    print('sigs done')
    return

# The rows in the WCET matrix are signature entries.
# The columns in the WCET are different components (A_data,
# m_L1, p, etc.)
def wcetGenMatrix():
    # Iterate through the data array; wait until the next data line is
    # complete (terminated).
    global wcet
    with open("wcet.csv", "w", newline='') as csvfile:
        writer = csv.writer(csvfile)
        i = 0
        while not (i >= largestMsgId and isComplete == True):
            # Check if next data line exists and is complete
            if i >= data.shape[0]:
                #print("a")
                time.sleep(0.1)
                continue
            while not checkDone(data[i]):
                #print("b")
                time.sleep(0.1)
            line = np.copy(data[i])
            line[labels["Miss?  "][1]] = getDepData(i, labels["Miss?  "][1])
            # Calculate the component delays for line i.
            delays = findCompDelays(line)
            # Find the signature to index the wcet array
            normLine = np.where(line > 0, 1, line)
            # Find the row in the wcet array to compare against
            key = tuple(normLine[1:])
            while key not in sigIndices:
                #print("Waiting for key to enter sigIndices...")
                time.sleep(0.1)
            dictIndex = sigIndices[key]
            # Check if the wcet even contains this index (i.e. too small)
            wcetRows = np.shape(wcet)[0]
            if (wcetRows - 1) < dictIndex:
                # expand wcet to include the index
                diff = dictIndex - (wcetRows - 1)
                wcet = np.pad(wcet, [(0, diff), (0, 0)])
            # Get the wcet entry corresponding to the line's signature
            wcetLine = wcet[dictIndex]
            # add to the count at the end of the line
            wcetLine[-1] += 1
            # Check if any entries in our line are greater than the wcet entry
            lineDiff = wcetLine - delays
            largerIndices = np.where(lineDiff < 0)[0]
            #print(line)
            # For all such enetries, write the newer value to wcet array
            for largerIndex in largerIndices:
                wcetLine[largerIndex] = delays[largerIndex]
            #print(wcetLine)
            #print(line)
            i += 1
        np.savetxt("wcet.csv", wcet, delimiter=",", fmt="%d")
    print('wcet done')
    return

# If a line is dependent on another, go to the source of that dependency
# and return the line/data of the source of the dependency.
def getDepData(i, targetCol):
    while data[i][labels["L1WBsrc"][0]] > 0:
        # Make sure the line is complete before doing this
        while not checkDone(data[i]):
            print("wait")
            time.sleep(0.1) 
        # Find the index of the line's immediate dependency
        j = int(data[i][labels["L1WBsrc"][0]] - 1)
        if j == i:
            break
        # Index the line's immediate dependency
        i = j
    return data[i][int(targetCol)]

def getDepLine(i):
    while data[i][labels["L1WBsrc"][0]] > 0:
        # Make sure the line is complete before doing this
        while not checkDone(data[i]):
            print("wait")
            time.sleep(0.1) 
        # Find the index of the line's immediate dependency
        j = int(data[i][labels["L1WBsrc"][0]] - 1)
        if j == i:
            break
        # Index the line's immediate dependency
        i = j
    return data[i]

def main():
    global isComplete
    csvThread = threading.Thread(target=csvWrite)
    csvThread.start()
    sigThread = threading.Thread(target=sigGenMatrix)
    sigThread.start()
    wcetThread = threading.Thread(target=wcetGenMatrix)
    wcetThread.start()
    for line in stdin:
        if (line.rstrip() == "End"):
            break;
        #print(line.rstrip())
        parseLine(line.rstrip())
    # Set var to done so the csv thread can stop
    isComplete = True
    csvThread.join()
    sigThread.join()
    wcetThread.join()
    #print(data)
    #np.savetxt("data.csv", data,delimiter=",", fmt="%d")

main()
