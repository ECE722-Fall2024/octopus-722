#!/bin/sh
# Mohamed Hassan
# Oct, 2023
# mohamed.hassan@mcmaster.ca
#This script takes as input two paramters in the following order:
# 1: is the configuration xml file 
# 2: is the source code c file for the benchmark
# the following is an example: 
# ./compile_and_run.sh tools/x86_trace_generator/LAB3_Configurations/lab3_1_8KB_64B_1_RANDOM_32KB_64_2_LRU.xml tools/x86_trace_generator/LAB3_BMs/cache_test_4KB_4_1.c



filename="${2##*/}"
BM="${2%.*}"
dirname="${2%/*}"
echo $BM
rm -rf $BM
gcc -o $BM\_exe $2

./tools/x86_trace_generator/pin-3.13-98189-g60a6ef199-gcc-linux/pin -t tools/x86_trace_generator/obj-intel64/trace_generator.so  -manual 1 -thread 1  -- $BM\_exe
mkdir $BM
mv memtrace.out $BM/trace_C0.trc.shared
./waf configure
./run.sh $1 $BM 2>&1 >/dev/null | python3 lab_parser.py > $BM/labdata.csv
cp -a $BM/labdata.csv $BM/labdata_tmp.csv
awk '!a[$0]++' $BM/labdata_tmp.csv > $BM/labdata.csv
rm -rf $BM/labdata_tmp.csv

execTime=$( tail -1 $BM/labdata.csv | cut -d, -f6)
requests=$( tail -1 $BM/labdata.csv | cut -d, -f1)
misses=$( grep -c "L1: Was the request a miss?,0,1,boolean" $BM/labdata.csv)
hits=$(expr $requests - $misses)

echo $BM
echo "Execution time: $execTime"             > $BM/summary.txt
echo "total number of requests: $requests"  >> $BM/summary.txt
echo "total number of hits:     $hits"      >> $BM/summary.txt
echo "total number of misses:   $misses"    >> $BM/summary.txt


echo "Execution time: $execTime"           
echo "total number of requests: $requests"  
echo "total number of hits:     $hits"      
echo "total number of misses:   $misses"    


