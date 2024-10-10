#
# File  :      do_sim_run.sh
# Author:      Salah Hessien
# Email :      salahga@mcmaster.ca
#
# Created On Dec 10, 2020
#


#!/bin/bash

# how to run the script
# 1) Script arguments:
#         ./do_sim_run.sh <TestType> <TestCategory> <TestCaseNum> <CopyLogFileFlag> <LogFileVersion>
#               <TestType>        : It can be either "Directed" or "Regression".
#               <TestCategory>    : It can be either "EEMbcTrace" or "SplashTrace".
#               <TestCaseNum>     : This argument used when running the script in "TestType = Directed" to 
#                                   select a certain testcase number from "TestCategory".
#               <CopyLogFileFlag> : A flag used to enable copying test results into a certain
#                                   version directory
#               <LogFileVersion>  : Final destination folder of the Log results
# ------------------------------------------------------
# 2) Example:
#    2.1) Directed Mode without log file copying
#            ./do_sim_run.sh Directed EEMbcTrace 5 0 0
#            ./do_sim_run.sh Directed SplashTrace 3 0 0
# ------------------------------------------------------
#    2.2) Directed Mode with log file copying
#            ./do_sim_run.sh Directed EEMbcTrace 6 1 0
#            ./do_sim_run.sh Directed SplashTrace 2 1 1
# -------------------------------------------------------
#    2.2) Regression Mode with log file copying
#            ./do_sim_run.sh Regression SynthTrace 0 1 0
#            ./do_sim_run.sh Regression EEMbcTrace 0 1 2
#            ./do_sim_run.sh Regression SplashTrace 0 1 3  
#

start="`date`"

# Benchmark base directory
BMsBasePath="/home/salah/shared/piscot-master/BMs"
BMsBasePath="../../../BMs"


# Log files base directory
LOGBasePath="./log"

# Detailed LogFile Gen Enable Flag
LOGFileEnableFlag=0

# script arguments
ConfigFilePath="$1"
TestType="$2"
TestCATG="$3"
TCNum=$4
CopyLOGFiles=$5
LOGFilesVersion="$6"

# simulator configuration file
#ConfigFilePath="./src/MultiCoreSim/model/test_cfg2.xml"
#ConfigFilePath="./src/MultiCoreSim/test/PISCOT/piscot_tc1.xml"

# internal definations
SplachDirName="splash"
EembcDirName="eembc-traces"
SynthDirName="synth"

# benchmark names
declare -a EEMbcTrace
declare -a SplashTrace

SynthTrace=("Random0Cycle" 
            "Random0CycleW"
            "Random20Cycle"
            "Stride64Bytes0CycleWrap"
            "Stride64Bytes0CycleWrapW"
            "Stride64Bytes20Cycle"
            "Stride64Bytes20CycleW"
            "Stride64Bytes20CycleWrap"
            "Stride64Random"
            "Stride512Bytes0Cycle"
            "Stride512Bytes0CycleW"
            "Stride512Bytes10Cycle")
   
ReplcTrace=("L2ReplcMFU100000"
            "L2Replc100KRand1"
            "L2Replc100KRand2"
            "L2Replc100KRand3"
            "L2Replc100KRand4"
            "L2Replc100KRand5"
            "L2Replc100KRand6"
            "L2Replc5")   
            
CohrTrace=("Cohr1RWNoCohr"
           "Cohr2RWNoCohr"
           "Cohr3RWNoCohr"
           "Cohr4RWNoCohr"
           "Cohr5RWNoCohr"
           "Cohr6RWNoCohr"
           "Cohr7RWNoCohr"
           "Cohr8RWNoCohr")
         
EEMbcTrace=("a2time01-trace" 
            "aifirf01-trace"
            "basefp01-trace"
            "cacheb01-trace"
            "empty-trace"
            "iirflt01-trace"
            "pntrch01-trace"
            "rspeed01-trace"
            "ttsprk01-trace")

#EEMbcTrace=("basefp01-trace"
#            "cacheb01-trace"
#            "empty-trace"
#            "iirflt01-trace"
#            "pntrch01-trace"
#            "rspeed01-trace"
#            "ttsprk01-trace")

#EEMbcTrace=("cacheb01-trace")

SplashTrace=("barnes"
             "cholesky"
             "fft"
             "fmm"
             "lu_non_contig"
             "lu_contig"
             "ocean"
             "radiosity"
             "radix"
             "raytrace"
             "volrend"
             "water_nsquared"
             "water_spatial")


#SplashTrace=("radix"
#             "volrend"
#             "water_nsquared"
#             "water_spatial"
#             "radiosity"
#             "raytrace")


LOGFileName="${LOGFilesVersion}_log.txt"


# check input configuration and run simulations
case "$TestType" in
   Directed)
      case "$TestCATG" in
         SynthTrace)
         TC=${SynthTrace[$TCNum]}
         BMPath="$BMsBasePath/$SynthDirName/$TC"
         LOGFilesPath="$LOGBasePath/$SynthDirName/$TC/$LOGFilesVersion"
         ;;
         ReplcTrace)
         TC=${ReplcTrace[$TCNum]}
         BMPath="$BMsBasePath/$SynthDirName/$TC"
         LOGFilesPath="$LOGBasePath/$SynthDirName/$TC/$LOGFilesVersion"
         ;;
         CohrTrace)
         TC=${CohrTrace[$TCNum]}
         BMPath="$BMsBasePath/$SynthDirName/$TC"
         LOGFilesPath="$LOGBasePath/$SynthDirName/$TC/$LOGFilesVersion"
         ;;
        EEMbcTrace)
         TC=${EEMbcTrace[$TCNum]}
         BMPath="$BMsBasePath/$EembcDirName/$TC"
         LOGFilesPath="$LOGBasePath/$EembcDirName/$TC/$LOGFilesVersion"
         ;;
        SplashTrace)
         TC=${SplashTrace[$TCNum]}
         BMPath="$BMsBasePath/$SplachDirName/$TC"
         LOGFilesPath="$LOGBasePath/$SplachDirName/$TC/$LOGFilesVersion"
         ;;
      esac

      LOGFileName="${LOGFilesVersion}_${TCNum}_log.txt"
      echo "Running Benchmark:"
      echo "         $BMPath"
      rm -rf "$BMPath/LatencyReport"*
      ./waf --run "scratch/MultiCoreSimulator --CfgFile=$ConfigFilePath --BMsPath=$BMPath --LogFileGenEnable=$LOGFileEnableFlag" --enable-gcov > "./$LOGFileName"
      if [ "$CopyLOGFiles" -eq 1 ];
        then
          mkdir -p $LOGFilesPath
          echo "Copy Log Files to $LOGFilesPath"
          cp "$BMPath/LatencyReport"* $LOGFilesPath
          #cp "$BMPath/LatencyTrace"* $LOGFilesPath
          cp $LOGFileName $LOGFilesPath
        fi
      ;;

   Regression)

      echo "Run Regression Test ...."

      case "$TestCATG" in
        SynthTrace)
           echo "---> Running Synth regressions"
           for TC in "${SynthTrace[@]}"
           do
             BMPath="$BMsBasePath/$SynthDirName/$TC"
             echo "Running Benchmark:"
             echo "         $BMPath"
             rm -rf "$BMPath/LatencyReport"*
             ./waf --run "scratch/MultiCoreSimulator --CfgFile=$ConfigFilePath --BMsPath=$BMPath --LogFileGenEnable=$LOGFileEnableFlag" > $LOGFileName

             if [ "$CopyLOGFiles" -eq 1 ];
             then
               LOGFilesPath="$LOGBasePath/$SynthDirName/$TC/$LOGFilesVersion"
               mkdir -p $LOGFilesPath
               echo "Copy Log Files to $LOGFilesPath"
               cp "$BMPath/LatencyReport"* $LOGFilesPath
               #cp "$BMPath/LatencyTrace"* $LOGFilesPath
               cp $LOGFileName $LOGFilesPath
             fi
           done
           ;; #SynthTrace
           
        EEMbcTrace)
           echo "---> Running MBC regressions"
           for TC in "${EEMbcTrace[@]}"
           do
             BMPath="$BMsBasePath/$EembcDirName/$TC"
             echo "Running Benchmark:"
             echo "         $BMPath"
             rm -rf "$BMPath/LatencyReport"*
             ./waf --run "scratch/MultiCoreSimulator --CfgFile=$ConfigFilePath --BMsPath=$BMPath --LogFileGenEnable=$LOGFileEnableFlag" > $LOGFileName

             if [ "$CopyLOGFiles" -eq 1 ];
             then
               LOGFilesPath="$LOGBasePath/$EembcDirName/$TC/$LOGFilesVersion"
               mkdir -p $LOGFilesPath
               echo "Copy Log Files to $LOGFilesPath"
               cp "$BMPath/LatencyReport"* $LOGFilesPath
               #cp "$BMPath/LatencyTrace"* $LOGFilesPath
               cp $LOGFileName $LOGFilesPath
             fi
           done
           ;; #EEMbcTrace

        ReplcTrace)
           echo "---> Running ReplcTrace regressions"
           for TC in "${ReplcTrace[@]}"
           do
             BMPath="$BMsBasePath/$SynthDirName/$TC"
             echo "Running Benchmark:"
             echo "         $BMPath"
             rm -rf "$BMPath/LatencyReport"*
             ./waf --run "scratch/MultiCoreSimulator --CfgFile=$ConfigFilePath --BMsPath=$BMPath --LogFileGenEnable=$LOGFileEnableFlag" > $LOGFileName

             if [ "$CopyLOGFiles" -eq 1 ];
             then
               LOGFilesPath="$LOGBasePath/$SynthDirName/$TC/$LOGFilesVersion"
               mkdir -p $LOGFilesPath
               echo "Copy Log Files to $LOGFilesPath"
               cp "$BMPath/LatencyReport"* $LOGFilesPath
               #cp "$BMPath/LatencyTrace"* $LOGFilesPath
               cp $LOGFileName $LOGFilesPath
             fi
           done
           ;; #ReplcTrace

         SplashTrace)
           echo "Running SPLASH regressions ...."
           for TC in "${SplashTrace[@]}"
           do
             BMPath="$BMsBasePath/$SplachDirName/$TC"
             echo "Running Benchmark:"
             echo "         $BMPath"
             ./waf --run "scratch/MultiCoreSimulator --CfgFile=$ConfigFilePath --BMsPath=$BMPath --LogFileGenEnable=$LOGFileEnableFlag" > $LOGFileName

             if [ "$CopyLOGFiles" -eq 1 ];
             then
               LOGFilesPath="$LOGBasePath/$SplachDirName/$TC/$LOGFilesVersion"
               mkdir -p $LOGFilesPath
               echo "Copy Log Files to $LOGFilesPath"
               cp "$BMPath/LatencyReport"* $LOGFilesPath
               #cp "$BMPath/LatencyTrace"* $LOGFilesPath
               cp $LOGFileName $LOGFilesPath
             fi
           done
           ;; #SplashTrace

      esac 
      ;; #Regression

esac


