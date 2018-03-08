#!/usr/bin/env python

import os, sys

fifo_path = '/var/tmp/a4m/socat_output_mtr_inj_fifo' #name of FIFO pipe
data_path = '/dev/shm/a4m_mtr_inj_input_data_file' #name of DATA file

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
OS_cmd = 'OS'
IO_cmd = 'IO'
PW_cmd = 'PW'
SV_cmd = 'SV'
PC_cmd = 'PC'
SO_cmd = 'SO'
RC_cmd = 'RC'

NRT1 = NRT2 = NRT3 = NRT4 = 0
PC1 = PC2 = PC3 = PC4 = 0
OS_P1 = OS_P2 = OS_P3 = OS_P4 = 0
OS_S1 = OS_S2 = OS_S3 = OS_S4 = 0
k_factor = 5000.0
mtr_factor = 1.0
conv_factor = 3785.412
alrm_pulse_count = alrm_pulse_time = 0
inj_vol = 25.0
control_method = 2
inj =  [
        ['101', 5000.0, 1.0, 0.0, 0.0, 0, 3785.412, 0, 0, 2, 0, 0, 0, 25.0],
        ['102', 5000.0, 1.0, 0.0, 0.0, 0, 3785.412, 0, 0, 2, 0, 0, 0, 25.0],
        ['103', 5000.0, 1.0, 0.0, 0.0, 0, 3785.412, 0, 0, 2, 0, 0, 0, 25.0],
        ['104', 5000.0, 1.0, 0.0, 0.0, 0, 3785.412, 0, 0, 2, 0, 0, 0, 25.0],
       ]
 
while True:

	fifo = open(fifo_path, "r")
	for line in fifo:
		print "\x1b[32;40mAccuLoad:  " + line + "\x1b[0m"
		addr = line[0:3]
                command = line[3:5]
                param = line[6:8]
                value = line[8:len(line)]


		if (command == IN_cmd):
			if addr == '301':
                                NRT1 += inj_vol / conv_factor
				PC1 += NRT1 * k_factor 
			elif addr == '302':
				NRT2 += 0.026
				PC2 += 133
			elif addr == '303':
				NRT3 += 0.026
				PC3 += 133
			elif addr == '304':
				NRT4 += 0.026
				PC4 += 133
			resp = "OK"

		elif (command == EP_cmd or command == DP_cmd):
			resp = "OK"

                elif (command == OS_cmd):
                        if param == 'P':
                            resp = "OS P %d" % OS_P1 
                        if param == 'S':
                            resp = "OS S %d" % OS_S1 

                elif (command == SO_cmd):
                        if param == 'P':
                            OS_P1 = int(value)
                        if param == 'S':
                            OS_S1 = int(value)
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

		elif (command == ST_cmd):
			resp = "ST 0000"

		elif (command == SV_cmd):
			resp = "SV 06 ABCDEF01"

                elif (command == CA_cmd):
		        if line[3:len(line)] == 'FFFF':
                            resp = "OK"

                elif (command == PW_cmd):
                        if param == '10':
                            k_factor = float(value)
                        if param == '11':
                            mtr_factor = float(value)
                        if param == '20':
                            hi_tol = float(value)
                        if param == '20':
                            lo_tol = float(value)
                        if param == '23':
                            conv_fact = float(value)
                        if param == '24':
                            alrm_pulse_count = int(value)
                        if param == '25':
                            alrm_pulse_time = int(value)
                        if param == '26':
                            control_method = int(value)
                        if param == '30':
                            inj_vol = float(value)
                        resp = "OK" 

                elif (command == PC_cmd):
                        resp = "PC %06d" % PC1
                elif (command == RC_cmd):
                        PC1 = 0
                        resp = "OK"

		else :
			resp = "NO00"

	print "\x1b[31;40mInjector:  " + addr + resp + "\x1b[0m"
	fifo.close()
	
	data = open(data_path, "w")
	data.write(addr + resp)
	data.close()

