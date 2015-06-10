#!/bin/bash

#the list of sizes to be used for dictionary size when compressing
sizes="256 257 511 512 1000 10000"

#test counter
t_counter=0

#setting working directory
lz78_path=$(dirname $0)
cd $lz78_path

export PATH=$PATH:$lz78_path

#if some file with same name of temporary file exist, delete them
if [ -e ../compressed ] ; then
	rm ../compressed
fi

if [ -e ../new.txt ] ; then
	rm ../new.txt
fi

for i in ${sizes} ; do
	#for each dimension, perform a test of all sample file
	for f in `ls ../samples` ; do
		t_counter=$(( $t_counter + 1 ))
		echo Test $t_counter
		../lz78 -c -i ../samples/$f -o ../compressed -s $i > /dev/null
		echo "../lz78 -c -i ../samples/$f -o ../compressed -s $i"
		if [ $? -eq 0 ] ; then 
			echo compressing $f ok
		else
			echo "Error while compressing"
		fi

		../lz78 -d -i ../compressed -o ../new.txt > /dev/null
		if [ $? -eq 0 ] ; then
			echo decompressing ok
		else
			echo "Error while decompressing"
		fi

		diff ../samples/$f ../new.txt > /dev/null

		if [ $? -eq 0 ] ; then
			echo -e "Test $t_counter OK\n"
		else
			echo -e "Test $t_counter FAIL\n"
			exit 1
		fi

		#deleting temporary files
		rm ../compressed ../new.txt
	done #for f in `ls ../samples`
done #for i in ${sizes}

echo "Everything OK"
