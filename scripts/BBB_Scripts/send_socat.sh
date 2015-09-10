#!/bin/sh

#Name:	send_socat.sh			
#
#Call Stack: systemd
#
#Inputs:
#	$1 - Board Type
#		a4m
#		a4b
#		a4i1
#		a4i2
#
#	File Read(/var/tmp/tiva_out) - Combined JSON Data Segments
#
#Outputs:
#	stdout - Combined JSON Segments
#
#Error Handling:
#	
#Notes:
#	This script will read all of the JSON segments from the /var/tmp/tiva_out file and will output
#	the data to its stdout. It will then clear the /var/tmp/tiva_out file.

#Arguement Checks
if [ "$1" != "a4m" ] && [ "$1" != "a4b" ] && [ "$1" != "a4i1" ] && [ "$1" != "a4i2" ]; then
	echo "Invalid Board Type"
	exit
fi

#File Locations
outfile=/var/tmp/$1/tiva_out

#Clear Anything Already in the Output File
> $outfile

seq=0
while true; do
	#Create the Header
	out=",\"ts\":"0","
	out=$out"\"seq\":"$seq","
	out=$out"\"id\":\"beaglebone\","
	out=$out"\"seg\":["

	#Read in the Entire File to Prevent Overwrites
	whole_file=`cat $outfile`
	#echo "$whole_file"
	if [ -z "$whole_file" ]; then
		#Empty File, Reset
		usleep 10000
		unset out
		continue
	fi
	#Read the Variable Line by Line
	while read -r text; do

		#Add Each Element
       		out="$out""$text"","	

	done < "$outfile"	

	#Remove the Last Comma
	out=$(echo "$out" | sed '$ s/,$//')
	out="$out""]}"
	
	echo "$out"
	
	#Clear the File for the Next Loop
	> $outfile

	seq=$((++seq))
	usleep 10000
done
