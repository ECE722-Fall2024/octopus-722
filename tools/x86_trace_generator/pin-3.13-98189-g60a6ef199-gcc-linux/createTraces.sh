#!/bin/sh
# This is a comment!


#first delete all exissssting raw files to regenerate clean:
 find $1 -name "*.raw" -type f -delete

for target in $(find $1 -type f ! -size 0 -exec grep -IL . "{}" \;);
do
   filename="${target##*/}"  # get filename
   dirname="${target%/*}" # get directory/path name
   echo "..........."
   echo $filename
   ./pin -t ../obj-intel64/trace_generator.so -tracename $target -- $target
   
done
