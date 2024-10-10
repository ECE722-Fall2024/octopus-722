# -*- coding: utf-8 -*-
"""
Created on Wed Mar 10 20:53:53 2021

@author: salahga
"""

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


TestResultBase = r'C:\Users\salahga\Desktop\vmsharingspace\ubuntu180404\thesis\results'
os.chdir(TestResultBase)


MbcTestNum      = 10
CoreNums        = 4
RunMode         = 'Directed'
RunMode         = 'Regression'
DirectedTCNum   = 0

TestCaT         = 'mbc' # or 'splash'
#TestCaT         = 'splash' # or 'splash'
#TestCaT         = 'synth'

# don't change these params
MbcDirName     = 'eembc-traces'
SynthDirName   = 'synth'
MbcDirName     = 'synth'
#SpashDirName = 'Splash'

cache2cache      = 'enable'

MbcTestCnt    = 12
SplashTestCnt = 11
ReplcTestCnt = 9

ReplcTrace=['L2ReplcMFU100000',
            'L2Replc100KRand1',
            'L2Replc100KRand2',
            'L2Replc100KRand3',
            'L2Replc100KRand4',
            'L2Replc100KRand5',
            'L2Replc100KRand6',
            'L2Replc5']

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

ReplcPolicy = ['LRU']

ReplcArbiter = ['FCFS-IO',
              'FCFS-IO',
              'FCFS-IO',
              'FCFS-IO',
              'FCFS-IO',
              'FCFS-OOO',
              'FCFS-OOO',
              'FCFS-OOO',
              'FCFS-OOO',
              'FCFS-OOO',
              'FCFS-IO',
              'FCFS-OOO',
              'FCFS-IO',
              'FCFS-OOO',
              'FCFS-IO',
              'FCFS-OOO',
              'FCFS-IO',
              'FCFS-OOO',
              'FCFS-IO',
              'FCFS-IO',
              'FCFS-IO',
              'FCFS-IO',
              'FCFS-IO',
              'FCFS-OOO',
              'FCFS-OOO',
              'FCFS-OOO',
              'FCFS-OOO',
              'FCFS-OOO',
              'FCFS-IO',
              'FCFS-OOO',
              'FCFS-IO',
              'FCFS-OOO',
              'FCFS-IO',
              'FCFS-OOO',
              'FCFS-IO',
              'FCFS-OOO']

L2RespBus = [50,
             50,
             50,
             50,
             50,
             50,
             50,
             50,
             50,
             50,
             50,
             50,
             50,
             50,
             50,
             50,
             50,
             50,
             25,
             25,
             25,
             25,
             25,
             25,
             25,
             25,
             25,
             25,
             25,
             25,
             25,
             25,
             25,
             25,
             25,
             25]

DRAMLat = [0,
           50,
           100,
           150,
           250,
           0,
           50,
           100,
           150,
           250,
           200,
           200,
           300,
           300,
           350,
           350,
           400,
           400,
           0,
           50,
           100,
           150,
           250,
           0,
           50,
           100,
           150,
           250,
           200,
           200,
           300,
           300,
           350,
           350,
           400,
           400]

ReplcArbiter = ['FCFS-IO',
              'FCFS-IO',
              'FCFS-IO',
              'FCFS-IO',
              'FCFS-IO',
              'FCFS-OOO',
              'FCFS-OOO',
              'FCFS-OOO',
              'FCFS-OOO',
              'FCFS-OOO',
              'FCFS-IO',
              'FCFS-OOO',
              'FCFS-IO',
              'FCFS-OOO',
              'FCFS-IO',
              'FCFS-OOO',
              'FCFS-IO',
              'FCFS-OOO',
              'FCFS-IO',
              'FCFS-IO',
              'FCFS-IO',
              'FCFS-IO',
              'FCFS-IO',
              'FCFS-OOO',
              'FCFS-OOO',
              'FCFS-OOO',
              'FCFS-OOO',
              'FCFS-OOO',
              'FCFS-IO',
              'FCFS-OOO',
              'FCFS-IO',
              'FCFS-OOO',
              'FCFS-IO',
              'FCFS-OOO',
              'FCFS-IO',
              'FCFS-OOO']

TCReshapedIdx = [0,1,2,3,10,4,12,14,16,5,6,7,8,11,9,13,15,17,
                 18,19,20,21,28,22,30,32,34,23,24,25,26,29,27,31,33,35]

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
  for kk in range(0,35+1):
    jj = TCReshapedIdx[kk]+1
    
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
                
        TCPath = BaseDirName + '/' + TCName + '/' + 'dramlat_tc' + str(jj) 
        TraceFile = TCPath + '/LatencyReport_C' + str(pp) + '.csv'
        ETFile = TCPath + '/dramlat_tc' + str(jj) + '_' + str(ii) + '_log.txt'
        
        print TraceFile
        
        if os.path.exists(ETFile):
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
            if L2Stats.size == 0:
                L2Stats=[0,0,0]
                ET=[(0,0),(0,0),(0,0),(0,0)]
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
            
            if chunk.size > 0:
                temp = pd.DataFrame (
                        {
                                'BM'         : [TCName],
                                'Approach'   : [ReplcArbiter[jj-1]],
                                'ReplcPolicy': [ReplcPolicy[0]],
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
                                'MissR'      : L2Stats[2],
                                'DRAMLat'    : DRAMLat[jj-1],
                                'L2RespLat'  : L2RespBus[jj-1]
                        }
                        , columns=['BM', 'Approach', 'ReplcPolicy', 'Cache2Cache', 'Core', 'NReq', 'Hits', 'Nreplc', 'WCReqL', 'WCRespL', 'WCReplc', 'WCL', 'WCLwtRepl', 'AvgL', 'AccPerR_L', 'AccReq_L', 'AccResp_L', 'AccReplc_L', 'CombL', 'ET' ,'L2Miss', 'L2Reqs', 'MissR', 'DRAMLat','L2RespLat']
                )
            
        
                df = pd.concat([df, temp])