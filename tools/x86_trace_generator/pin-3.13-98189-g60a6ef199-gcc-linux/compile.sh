
#!/bin/sh
# This is a comment!

for dir in ../../benchmarks/*/     # list directories in the form "/tmp/dirname/"
do

    #echo $dir
    BM=${dir%*/}      # remove the trailing "/"
    
    if [ "${BM##*/}" != "common" ] 
    then
       echo "${BM##*/}"    # print everything after the final "/"
       gcc $dir/*c -I ../../benchmarks/common/ -o $dir/BM
    fi
done


