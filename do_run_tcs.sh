#!/bin/bash

ConfigFileBase="./src/MultiCoreSim/test"

function runTests {
  TESTNAME=$1
  START=$2
  END=$3
  BMSUITE=$4
  RUNTYPE=$5
  BMTestCase=$6
  echo "Running $TESTNAME test:"
  echo "-------> BMs         : $BMSUITE "  
  echo "-------> NumTestCase : `expr $END - $START + 1` testcases" 
   
  for i in $(eval echo "{$START..$END}")
  do 
    ConfigFilePath="$ConfigFileBase/$TESTNAME/${TESTNAME}_tc$i.xml"
    echo "-------> CnfgFile    : $ConfigFilePath"
    ./do_sim_run.sh $ConfigFilePath $RUNTYPE $BMSUITE $BMTestCase 1 ${TESTNAME}_tc$i
  done
}

# Uncomment the following line to Run all SynthTrace Regression for all piscot testcases
#runTests piscot 1 18 SynthTrace Regression 0

# Uncomment the following line to Run all EEMBcTrace Regression for all piscot testcases
#runTests piscot 1 18 EEMbcTrace Regression 0

# Example to run a certain Benchmark with a certain configuration file
#runTests piscot 5 5 EEMbcTrace Directed 2

#runTests piscot 19 19 SplashTrace Directed 10

#runTests replc 1 7 ReplcTrace Directed 2

#runTests dramlat 9 10 SynthTrace Regression 0

#runTests dramlat 9 10 EEMbcTrace Regression 0

#for i in $(eval echo "{6..11}")
#do 
#  runTests dramlat 1 36 ReplcTrace Directed $i
#done

#runTests dramlat 1 36 SynthTrace Directed 11
 
#runTests l2latswap 1 16 SynthTrace Directed 8
#runTests arb 3 4 EEMbcTrace Directed 8
#runTests cohr 3 3 EEMbcTrace Directed 8

#runTests replc 17 17 ReplcTrace Directed 6

#runTests OOOswap 9 16 EEMbcTrace Directed 8

runTests cohr 3 3 EEMbcTrace Directed 0

# simulator row commands:
#./do_sim_run.sh ./src/MultiCoreSim/test/PISCOT/piscot_tc4.xml Regression SynthTrace 0 1 piscot_tc4

#./waf --run "scratch/MultiCoreSimulator --CfgFile=./src/MultiCoreSim/test/piscot/piscot_tc17.xml --BMsPath=../../../BMs/eembc-traces/iirflt01-trace --LogFileGenEnable=1"

#./waf --run scratch/MultiCoreSimulator --command-template="gdb --args %s --CfgFile=./src/MultiCoreSim/test/piscot/piscot_tc19.xml --BMsPath=../../../BMs/synth/L2Replc6 --LogFileGenEnable=0"

#./waf --run "scratch/MultiCoreSimulator --CfgFile=./src/MultiCoreSim/test/piscot/piscot_tc17.xml --BMsPath=../../../BMs/eembc-traces/iirflt01-trace --LogFileGenEnable=0" --enable-gcov
# lcov --capture --directory . --output-file cov.info
#genhtml cov.info --output-directory out

#rsyc command

