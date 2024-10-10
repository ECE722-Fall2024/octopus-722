# -*- coding: utf-8 -*-
"""
Created on Wed Mar 10 07:59:21 2021

@author: salahga
"""

import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.pyplot import *
from matplotlib import dates
import scipy.stats as stats
import datetime
import os

from collections import deque
from io import StringIO

TestResultBase = r'C:\Users\salahga\Desktop'
os.chdir(TestResultBase)


with open("smpl.txt","r") as fi:
    ET_Line = []
    L2_Line = []
    for ln in fi:
        if ln.startswith("Cpu"):
            ET_Line.append(ln)
        if ln.startswith("L2"):
            L2_Line.append(ln)
l = []
for i in ET_Line:
    for t in i.split():
        try:
            l.append(int(t))
        except ValueError:
            pass
    
B = np.reshape(l, (-1, 2))

ET = sorted(B, key=lambda x : x[0])

l = []
for i in L2_Line:
    for t in i.split():
        try:
            l.append(float(t))
        except ValueError:
            pass
    
B = np.reshape(l, (-1, 1))

L2Stats = B
L2Stats[0] = int(L2Stats[0])
L2Stats[1] = int(L2Stats[1])

TestResultBase = r'C:\Users\salahga\Desktop\vmsharingspace\ubuntu180404\thesis\results'
os.chdir(TestResultBase)


MbcTestNum      = 10
CoreNums        = 4
RunMode         = 'Directed'
RunMode         = 'Regression'
DirectedTCNum   = 8

TestCaT         = 'mbc' # or 'splash'
#TestCaT         = 'splash' # or 'splash'
TestCaT         = 'synth'

# don't change these params
MbcDirName     = 'eembc-traces'
SynthDirName   = 'synth'

#SpashDirName = 'Splash'

cache2cache      = 'enable'

MbcTestCnt    = 9
SplashTestCnt = 11
ReplcTestCnt = 9

ReplcTrace=['L2ReplcMFU100000',
            'L2Replc100KRand1',
            'L2Replc100KRand2',
            'L2Replc100KRand3',
            'L2Replc100KRand4',
            'L2Replc100KRand5',
            'L2Replc100KRand6',
            'L2Replc5',
            'L2Replc6']


MbcTests = [
            'Random0Cycle' ,
            'Random0CycleW',
            'Random20Cycle',
            'Stride64Bytes0CycleWrap',
            'Stride64Bytes0CycleWrapW',
            'Stride64Bytes20Cycle',
            'Stride64Bytes20CycleW',
            'Stride64Bytes20CycleWrap',
            'Stride64Random',
            'Stride512Bytes0Cycle',
            'Stride512Bytes0CycleW',
            'Stride512Bytes10Cycle']


MbcTests = [
           'a2time01-trace',
           'aifirf01-trace',
           'basefp01-trace',
           'cacheb01-trace',
           'empty-trace',
           'iirflt01-trace',
           'pntrch01-trace',
           'rspeed01-trace',
           'ttsprk01-trace']


SplashTests =['barnes',
             'cholesky',
             'fft',
             'fmm',
             'lu_non_contig',
             'ocean',
             'radix',    
             'volrend',
             'water_nsquared',
             'water_spatial']


SplashTests =['radix']

CSVHeader=["ReqId",
           "ReqAddr",
           "RefTraceCycle",
           "ReqBusLatency",
           "RespBusLatency",
           "ReplcLatency",
           "LatencyMinusRepl",
           "TotLatency",
           "WcReqLatency",
           "WcRespLatency",
           "WcReplcLatency",
           "WcLatency",
           "WcLatencyMinsRepl",
           "AverageLatency"]

chunksize = 1000000
df = pd.DataFrame()

ReplcPolicy = ['RAND',
             'LRU',
             'MRU',
             'LFU',
             'MFU',
             'FIFO',
             'LIFO',
             'RAND',
             'LRU',
             'MRU',
             'LFU',
             'MFU',
             'FIFO',
             'LIFO',
             'RAND',
             'LRU',
             'MRU',
             'LFU',
             'MFU',
             'FIFO',
             'LIFO'
             ]

ReplcArbiter = ['PISCOT',
              'PISCOT',
              'PISCOT',
              'PISCOT',
              'PISCOT',
              'PISCOT',
              'PISCOT',
              'PISCOT',
              'PISCOT',
              'PISCOT',
              'PISCOT',
              'PISCOT',
              'PISCOT',
              'PISCOT',
              'MSI',
              'MSI',
              'MSI',
              'MSI',
              'MSI',
              'MSI',
              'MSI']

if RunMode == 'Directed':
    TCNums = 1
else:
    if TestCaT == 'mbc':
        TCNums = MbcTestCnt
    elif TestCaT == 'synth':
        TCNums = ReplcTestCnt
    else:
        TCNums = SplashTestCnt
        
# loop over trace files
for ii in range (TCNums):
  for jj in range(8+7,14+7+1):
  #for jj in range(1,7+1):
    for pp in range (CoreNums):
        chunkHit = 0
        accPerReqLat = 0
        accReqLat = 0
        accRespLat = 0
        accReplcLat = 0
        Nreq = 0
        Nreplc = 0
        
        if TestCaT == 'mbc':
            BaseDirName = MbcDirName
            if RunMode == 'Directed':
                TCName = MbcTests[DirectedTCNum] 
            else:
                TCName = MbcTests[ii] 
        elif TestCaT == 'synth':
            BaseDirName = SynthDirName
            if RunMode == 'Directed':
                TCName = ReplcTrace[DirectedTCNum] 
            else:
                TCName = ReplcTrace[ii] 
        else:
            BaseDirName = SpashDirName
            if RunMode == 'Directed':
                TCName = SplashTests[DirectedTCNum] 
            else:
                TCName = SplashTests[ii]
                
        TCPath = BaseDirName + '/' + TCName + '/' + 'replc_tc' + str(jj) 
        TraceFile = TCPath + '/LatencyReport_C' + str(pp) + '.csv'
        ETFile = TCPath + '/replc_tc' + str(jj) + '_' + str(ii) + '_log.txt'
        
        print TraceFile
        
        with open(ETFile,"r") as fi:
            ET_Line = []
            L2_Line = []
            for ln in fi:
                if ln.startswith("Cpu"):
                    ET_Line.append(ln)
                if ln.startswith("L2"):
                    L2_Line.append(ln)
        l = []
        for i in ET_Line:
            for t in i.split():
                try:
                    l.append(int(t))
                except ValueError:
                    pass
            
        B = np.reshape(l, (-1, 2))
        
        ET = sorted(B, key=lambda x : x[0])

        l = []
        for i in L2_Line:
            for t in i.split():
                try:
                    l.append(float(t))
                except ValueError:
                    pass
            
        B = np.reshape(l, (-1, 1))
        
        L2Stats = B
        L2Stats[0] = int(L2Stats[0])
        L2Stats[1] = int(L2Stats[1])      
        L2Stats[2] = L2Stats[2]/100.0  
        
        for chunk in pd.read_csv(TraceFile, chunksize=chunksize):
            chunkHit     = chunkHit     + (chunk['TotLatency'] == 1).astype(int).sum()
            accPerReqLat = accPerReqLat + chunk['TotLatency'].sum()
            accReqLat    = accReqLat    + chunk['ReqBusLatency'].sum()
            accRespLat   = accRespLat   + chunk['RespBusLatency'].sum()
            accReplcLat  = accReplcLat  + chunk['ReplcLatency'].sum()
            Nreq         = Nreq         + chunk['ReplcLatency'].count()
            Nreplc       = Nreplc       + (chunk['ReplcLatency'] != 0).astype(int).sum()
        
        temp = pd.DataFrame (
                {
                        'BM'         : [TCName],
                        'Approach'   : [ReplcArbiter[jj-1]],
                        'ReplcPolicy': [ReplcPolicy[jj-1]],
                        'Cache2Cache': [cache2cache],
                        'Core'       : [pp],
                        'NReq'       : [Nreq],
                        'Hits'       : [chunkHit],
                        'Nreplc'     : [Nreplc],
                        'WCReqL'     : [chunk.iloc[-1].WcReqLatency],
                        'WCRespL'    : [chunk.iloc[-1].WcRespLatency],
                        'WCReplc'    : [chunk.iloc[-1].WcReplcLatency],
                        'WCL'        : [chunk.iloc[-1].WcLatency],
                        'WCLwtRepl'  : [chunk.iloc[-1].WcLatencyMinsRepl],
                        'AvgL'       : [chunk.iloc[-1].AverageLatency],
                        'AccPerR_L'  : [accPerReqLat],
                        'AccReq_L'   : [accReqLat],
                        'AccResp_L'  : [accRespLat],
                        'AccReplc_L' : [accReplcLat],
                        'CombL'      : [chunk.iloc[-1].RefTraceCycle],
                        'ET'         : ET[pp][1],
                        'L2Miss'     : L2Stats[0],
                        'L2Reqs'     : L2Stats[1],
                        'MissR'      : L2Stats[2]
                }
                , columns=['BM', 'Approach', 'ReplcPolicy', 'Cache2Cache', 'Core', 'NReq', 'Hits', 'Nreplc', 'WCReqL', 'WCRespL', 'WCReplc', 'WCL', 'WCLwtRepl', 'AvgL', 'AccPerR_L', 'AccReq_L', 'AccResp_L', 'AccReplc_L', 'CombL', 'ET' ,'L2Miss', 'L2Reqs', 'MissR']
        )
        
    
        df = pd.concat([df, temp])