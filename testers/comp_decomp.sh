#!/bin/bash

root="../"

#the list of sizes to be used for dictionary size when compressing
sizes="2 1000 20000"

#test counter
t_counter=0

#setting working directory
cd "$(dirname "$0")"

for i in ${sizes} ; do
	#for each dimension, perform a test of all sample file
	for f in `ls ../samples` ; do
		t_counter=$(( $t_counter + 1 ))
		echo Test $t_counter
		../lz78 -c -i ../samples/$f -o ../compressed -s $i
		if [ $? -eq 0 ] ; then 
			echo compressing ok
		else
			echo Error while compressing
		fi

		../lz78 -d -i ../compressed -o ../new.txt
		if [ $? -eq 0 ] ; then
			echo decompressing ok
		else
			echo Error while decompressing
		fi

		diff ../samples/$f ../new.txt 

		if [ $? -eq 0 ] ; then
			echo -e "Test $t_counter OK\n"
		else
			echo -e "Test $t_counter FAIL\n"
		fi

		#deleting temporary files
		rm ../compressed ../new.txt
	done
done
