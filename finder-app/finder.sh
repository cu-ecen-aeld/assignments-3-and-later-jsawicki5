#!/bin/bash

fileDir=$1
searchStr=$2
numFiles=0
count=0

if [ $# != 2 ]
then
	echo "Must submit 2 parameters, <fileDir> then <searchStr>"
	exit 1
elif [ -d ${fileDir} ]
then
	cd ${fileDir}
else
	echo "${fileDir} does not represent a directory on the filesystem"
        exit 1
fi

while read -r line; do
        subCount=$(echo ${line} | cut -d: -f2)
        if [ ${subCount} -ne 0 ]
        then
                numFiles=$((numFiles + 1))
        	count=$((count + subCount))
	fi
done < <(grep -r -c ${searchStr} *)
echo "The number of files are ${numFiles} and the number of matching lines are ${count}"
exit 0

