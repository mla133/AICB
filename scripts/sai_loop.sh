#!/bin/bash    
fifo_name="/var/tmp/a4m/socat_output_serial_fifo1"
while true
do
    if read line; then
        echo $line > /var/tmp/serial_input_data_file
    fi
done <"$fifo_name"
