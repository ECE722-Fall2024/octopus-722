#!/bin/sh
# This is a comment!


#first delete all exissssting raw files to regenerate clean:
filename="${1##*/}"
BM="${1%.*}"
dirname="${1%/*}"
echo $BM
rm -rf $BM
gcc -o $BM\_exe $1
./pin -t ../obj-intel64/trace_generator.so  -manual 1 -thread 1  -- $BM\_exe
mkdir $BM
mv memtrace.out $BM/trace_C0.trc.shared
'''
for target in $(find $1 -type f ! -size 0 -exec grep -IL . "{}" \;);
do
   filename="${target##*/}"  # get filename
   dirname="${target%/*}" # get directory/path name
   echo "..........."
   echo $filename
   ./pin -t ../obj-intel64/trace_generator.so -tracename $target -- $target
   
done
