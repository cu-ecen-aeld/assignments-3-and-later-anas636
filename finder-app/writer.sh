#!/bin/bash

if [ "$#" -ne  2 ]; then
   
   echo "Two argument must be entered $0 argument1 argument2 "
   echo "The first argument must be a full path to a file (including filename) on the filesystem"
   echo "The second argument must be a text string which will be written within this file"
   
   exit 1
fi

writefile=$1
writestr=$2

file_path=$(echo $writefile | awk -F/ '{NF=NF-1;$NF=$NF"/"}1' OFS=/)

 
mkdir -p $file_path && echo $writestr > $writefile

