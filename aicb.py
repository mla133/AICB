#!/usr/bin/env python

import os, sys

fifo_path = '/var/tmp/a4m/socat_output_mtr_inj_fifo' #name of FIFO pipe
data_path = '/dev/shm/a4m_mtr_inj_input_data_file' #name of DATA file

# Lists of AICB commands
IN_cmd = 'IN'
EQ_cmd = 'EQ'
AI_cmd = 'AI'
DI_cmd = 'DI'
CA_cmd = 'CA FFFF'
ST_cmd = 'ST'
TS_cmd = 'TS'
RC_cmd = 'RC'
EP_cmd = 'EP'
DP_cmd = 'DP'
PC_cmd = 'PC'
OS_cmd = 'OS'
IO_cmd = 'IO'
PW_cmd = 'PW'
SV_cmd = 'SV'

NRT1 = NRT2 = NRT3 = NRT4 = NRT5 = 0
PC1 = PC2 = PC3 = PC4 = PC5 = 0

while True:

	fifo = open(fifo_path, "r")
	for line in fifo:
		print "\x1b[32;40mAccuLoad:  " + line + "\x1b[0m"
		addr = line[0:3]
		command = line[3:len(line)]
		
		if (command == IN_cmd):
			if addr == '301':
				NRT1 += 0.026
				PC1 += 133
			elif addr == '302':
				NRT2 += 0.026
				PC2 += 133
			elif addr == '303':
				NRT3 += 0.026
				PC3 += 133
			elif addr == '304':
				NRT4 += 0.026
				PC4 += 133
			elif addr == '305':
				NRT5 += 0.026
				PC5 += 133
			resp = "OK"

		elif (command == EP_cmd or command == DP_cmd):
			resp = "OK"

		elif (command == TS_cmd):
			if addr == '301':
				resp = "TS %12.3f 0000" % NRT1
			if addr == '302':
				resp = "TS %12.3f 0000" % NRT2
			if addr == '303':
				resp = "TS %12.3f 0000" % NRT3
			if addr == '304':
				resp = "TS %12.3f 0000" % NRT4
			if addr == '305':
				resp = "TS %12.3f 0000" % NRT5

		elif (command == ST_cmd):
			resp = "ST 0000"

		elif (command == SV_cmd):
			resp = "SV 06 ABCDEF01"

                elif (command == CA_cmd):
                        resp = "OK"

                elif (command == PC_cmd):
                        resp = "OK" 

		else :
			resp = "NO00"

	print "\x1b[31;40mInjector:  " + addr + resp + "\x1b[0m"
	fifo.close()
	
	data = open(data_path, "w")
	data.write(addr + resp)
	data.close()

