#!/bin/sh


if [ "$#" -ne  2 ]; then
   
   echo "Two argument must be entered $0 argument1 argument2 "
   echo "The first argument must be path to a directory on the filesystem"
   echo "The second argument must be a text string which will be searched within these files in the directory and all subdirectories"
   
   exit 1
fi

filesdir=$1
searchstr=$2


if [[ -d $filesdir ]]; then

	match_lines=$(grep -R $searchstr $filesdir | wc -l)
	numb_files=$(find $filesdir -type f | wc -l)
	
	#echo "The number of files are $numb_files and the number of matching lines are $match_lines" | tee -a /tmp/assignment4-result.txt
	

else
	echo "$filesdir is not a directory on the filesystem"
	exit 1

fi
