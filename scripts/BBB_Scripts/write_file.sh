#!/bin/sh

#Name:	write_file.sh			
#
#Inputs:
#	stdin - The JSON Output of json_write from a Direct Pipe
#	
#
#	$1 - The packet type:
#		d - Data
#		s - Serial
#		mi - Metered Injector
#		cmd - Command
#
#	$2 - Board Type
#		a4m
#		a4b
#		a4i1
#		a4i2
#
#	$3 - The Instance Number (ie Printer 1 or Printer 2)
#
#Outputs:
#	File Write(/var/tmp/tiva_out) - JSON Data Segmen
#
#Error Handling:
#	
#Notes:
#	Each json_write will have its output piped into this script.
#	This script will make write the output to a text file common to all json_write processes
#	The script will check to make sure the file has been read before trying to write again.
#	The write lock scripts will not overwrite eachother in the comman file due to 
# 	a file write being atomic in Linux if it is under 4K bytes. 
#	(http://pubs.opengroup.org/onlinepubs/007904975/functions/write.html)

#File Locations
if [ "$2" != "a4m" ] && [ "$2" != "a4b" ] && [ "$2" != "a4i1" ] && [ "$2" != "a4i2" ]; then
	echo "Invalid Board Type"
	exit
fi
outfile=/var/tmp/$2/tiva_out

#If the Data Pipe is Restarted
#Remove Any Old Packets
if [ "$2" = "d" ]; then
	rm "$outfile"
fi

touch $outfile

while read -r input; do
	#Format the Type and Instance Strings
	type="\"type\":\"$1\""
	ins="\"ins\":$3"
		
	#Make Sure We Haven't Already Written to the File 
	#Before it is Read
	if ! grep -q $type.*$ins $outfile; then
		#Append the Input to the Main File
		echo "$input" >> $outfile
		echo "$input"  
	fi
	usleep 10000
done
	#rm -rf "$lockdir"

