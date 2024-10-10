# lab_parser.py -- for use with CE 4DM4
# This script takes in the timestamp entries and adds in the value cell and the
# explanation cell. It also filters out pending (-2) lines and (termina lines?)

from sys import stdin
import sys
import csv

glossary = {}

def main():
    # Read glossary file and read data into dictionary
    csvwriter = csv.writer(sys.stdout)
    with open("parser/glossary.csv","r",newline="") as csvfile:
        csvreader = csv.DictReader(csvfile)
        for row in csvreader:
            label = row["label"]
            entry = (row["value"],[])
            level = 0 if row["level"] == "L1" else 1
            desc = row["description"]

            if label not in glossary:
                glossary[label] = entry

            while len(glossary[label][1]) <= level:
                glossary[label][1].append(None)

            glossary[label][1][level] = desc

    for line in stdin:
        if (line == "End\n"):
            break
        fields = line.strip().split(',')
        coreIndex = 1 if fields[-2] == '10' else 0
        # Convert addresses to hex
        fields[1] = hex(int(fields[1]))[2:]

        if fields[-2] == '10':
            fields[-2] = 'LLC'
        if fields[-1] == '-2' or fields[-1] == '-1' or fields[2] == 'termina':
            continue
        # Insert glossary explanation
        fields.insert(3,glossary[fields[2]][1][coreIndex])
        # Insert value explanation
        fields.append(glossary[fields[2]][0])
        csvwriter.writerow(fields)
main()
