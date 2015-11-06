#!/usr/bin/env python

import os, sys

fifo_path = '/var/tmp/a4m/socat_output_serial_fifo3' #name of FIFO pipe
data_path = '/var/tmp/a4m/serial_input_data_file3' #name of DATA file

# Lists of AICB commands
IN_cmd = 'IN'
EQ_cmd = 'EQ'
AI_cmd = 'AI'
DI_cmd = 'DI'
CA_cmd = 'CA'
ST_cmd = 'ST'
TS_cmd = 'TS'
RC_cmd = 'RC'
EP_cmd = 'EP'
DP_cmd = 'DP'
PC_cmd = 'PC'
OS_P_cmd = 'OS P'
OS_S_cmd = 'OS S'
IO_cmd = 'IO'
PW_cmd = 'PW'
SV_cmd = 'SV'

NRT1 = NRT2 = NRT3 = NRT4 = NRT5 = 0
PC1 = PC2 = PC3 = PC4 = PC5 = 0

while True:

	fifo = open(fifo_path, "r")
	for line in fifo:
		print "\x1b[32;40mReceived: " + line + "\x1b[0m"
		addr = line[0:3]
		command = line[3:len(line)]
		
		if (command == IN_cmd):
			if addr == '101':
				NRT1 += 0.026
				PC1 += 133
			elif addr == '102':
				NRT2 += 0.026
				PC2 += 133
			elif addr == '103':
				NRT3 += 0.026
				PC3 += 133
			elif addr == '104':
				NRT4 += 0.026
				PC4 += 133
			elif addr == '105':
				NRT5 += 0.026
				PC5 += 133
			resp = "OK"

		elif (command == EP_cmd or command == DP_cmd):
			resp = "OK"

		elif (command == TS_cmd):
			if addr == '101':
				resp = "TS %12.3f 0000" % NRT1
			if addr == '102':
				resp = "TS %12.3f 0000" % NRT2
			if addr == '103':
				resp = "TS %12.3f 0000" % NRT3
			if addr == '104':
				resp = "TS %12.3f 0000" % NRT4
			if addr == '105':
				resp = "TS %12.3f 0000" % NRT5

		elif (command == ST_cmd):
			resp = "ST 0000"

		elif (command == SV_cmd):
			resp = "SV 06 ABCDEF01"

		else :
			resp = "NO00"

	print "\x1b[31;40mReponse:  " + addr + resp + "\x1b[0m"
	fifo.close()
	
	data = open(data_path, "w")
	data.write(addr + resp)
	data.close()

