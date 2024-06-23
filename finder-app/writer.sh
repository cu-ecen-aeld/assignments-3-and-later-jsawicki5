#!/bin/bash

writefile=$1
writestr=$2

if [ $# -ne 2 ]
then
	echo "script should be executed with 2 arguments"
	echo "ex. writer.sh <writefile> <writestr>"
	echo "writefile = full path to a file"
	echo "writestr = the string to be written to the file"

	exit 1
fi

filepath=$(dirname ${writefile})
filename=$(basename ${writefile})
mkdir ${filepath}
if [ -d ${filepath} ]
then
	cd ${filepath}
	touch ${filename}
else
	echo "failed to create directory"
fi

if [ -f ${writefile} ]
then
	echo "${writestr}" > ${writefile}
	echo "file ${filename} write completed successfully"
	exit 0
else
	echo "file ${writefile} could not be created"
	exit 1
fi


