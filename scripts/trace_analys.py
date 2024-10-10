# -*- coding: utf-8 -*-
"""
Created on Fri Apr  3 23:01:04 2020

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

TestResultBase = r'C:\Users\salahga\Desktop\vmsharingspace\ubuntu180404\BMs'
TestResultBase = r'C:\Users\salahga\McMaster University\Hassan, Mohamed - PMSI Unified TDM\EMbc'

os.chdir(TestResultBase)

# set script params
TestResultVersion = 9
cache2cache      = 'enable'
TestResultVersion = 2
cache2cache      = 'disable'

TestResultVersion = 5
cache2cache      = 'disable'

TestResultVersion = 13
cache2cache      = 'disable'

TestResultVersion = 17
cache2cache      = 'disable'

TestResultVersion = 1
cache2cache      = 'disable'



#TestResultVersion = 6
#cache2cache      = 'enable'


MbcTestNum      = 10
CoreNums        = 4
RunMode         = 'Directed'
RunMode         = 'Regression'
DirectedTCNum   = 8

TestCaT         = 'mbc' # or 'splash'
#TestCaT         = 'splash' # or 'splash'

# don't change these params
MbcDirName   = 'eembc-traces'
#MbcDirName   = 'synth'

#SpashDirName = 'Splash'

MbcTestCnt    = 9
SplashTestCnt = 11

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

if RunMode == 'Directed':
    TCNums = 1
else:
    if TestCaT == 'mbc':
        TCNums = MbcTestCnt
    else:
        TCNums = SplashTestCnt
        
# loop over trace files
for ii in range (TCNums):

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
        else:
            BaseDirName = SpashDirName
            if RunMode == 'Directed':
                TCName = SplashTests[DirectedTCNum] 
            else:
                TCName = SplashTests[ii]
                
        TCPath = BaseDirName + '/' + TCName + '/' + str(TestResultVersion) 
        TraceFile = TCPath + '/LatencyReport_C' + str(pp) + '.csv'
        
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
                        'Approach'   : ['PISCOT'],
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
                        'CombL'      : [chunk.iloc[-1].RefTraceCycle]
                }
                , columns=['BM', 'Approach', 'Cache2Cache', 'Core', 'NReq', 'Hits', 'Nreplc', 'WCReqL', 'WCRespL', 'WCReplc', 'WCL', 'WCLwtRepl', 'AvgL', 'AccPerR_L', 'AccReq_L', 'AccResp_L', 'AccReplc_L', 'CombL']
        )
        
    
        df = pd.concat([df, temp])

