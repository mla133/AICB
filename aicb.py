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

OS_P1 = OS_P2 = OS_P3 = OS_P4 = 0
OS_S1 = OS_S2 = OS_S3 = OS_S4 = 0
alrm_pulse_count = alrm_pulse_time = 0
inj_addr = 0
#         0   ,      1,   2,   3,   4, 5,        6, 7, 8, 9,10,11,12,   13,  14,15
inj =  [
        ['301', 5000.0, 1.0, 0.0, 0.0, 0, 3785.412, 0, 0, 2, 0, 0, 0, 25.0, 0.0, 0],
        ['302', 5000.0, 1.0, 0.0, 0.0, 0, 3785.412, 0, 0, 2, 0, 0, 0, 25.0, 0.0, 0],
        ['303', 5000.0, 1.0, 0.0, 0.0, 0, 3785.412, 0, 0, 2, 0, 0, 0, 25.0, 0.0, 0],
        ['304', 5000.0, 1.0, 0.0, 0.0, 0, 3785.412, 0, 0, 2, 0, 0, 0, 25.0, 0.0, 0],
       ]
 
while True:

	fifo = open(fifo_path, "r")
	for line in fifo:
		print "\x1b[32;40mAccuLoad:  " + line + "\x1b[0m"
		addr = line[0:3]
                command = line[3:5]
                param = line[6:8]
                value = line[8:len(line)]
                inj_addr = (int)(addr)-301

		if (command == IN_cmd):
                        # NRT (14) += INJ_VOL (13) / CONV_FACT (6)
                        # PC (15) += NRT (14) * K_FACT (1)
			if addr == '301':
                                inj[inj_addr][14] += inj[inj_addr][13] / inj[inj_addr][6]
				inj[inj_addr][15] += inj[inj_addr][14] * inj[inj_addr][1] 
			resp = "OK"

		elif (command == EP_cmd or command == DP_cmd):
                        resp = "OK"

                elif (command == OS_cmd):
                        if addr == '301':
                            if param == 'P':
                                resp = "OS P %d" % OS_P1 
                            if param == 'S':
                                resp = "OS S %d" % OS_S1 
                        if addr == '302':
                            if param == 'P':
                                resp = "OS P %d" % OS_P2 
                            if param == 'S':
                                resp = "OS S %d" % OS_S2 
                        if addr == '303':
                            if param == 'P':
                                resp = "OS P %d" % OS_P3 
                            if param == 'S':
                                resp = "OS S %d" % OS_S3 
                        if addr == '304':
                            if param == 'P':
                                resp = "OS P %d" % OS_P4 
                            if param == 'S':
                                resp = "OS S %d" % OS_S4 

                elif (command == SO_cmd):
                        if addr == '301':
                            if param == 'P':
                                OS_P1 = int(value)
                            if param == 'S':
                                OS_S1 = int(value)
                        if addr == '302':
                            if param == 'P':
                                OS_P2 = int(value)
                            if param == 'S':
                                OS_S2 = int(value)
                        if addr == '303':
                            if param == 'P':
                                OS_P3 = int(value)
                            if param == 'S':
                                OS_S3 = int(value)
                        if addr == '304':
                            if param == 'P':
                                OS_P4 = int(value)
                            if param == 'S':
                                OS_S4 = int(value)

                        resp = "OK"

		elif (command == TS_cmd):
                        resp = "TS %12.3f 0000" % (float)(inj[(int)(addr)-301][14])

		elif (command == ST_cmd):
			resp = "ST 0000"

		elif (command == SV_cmd):
			resp = "SV 06 ABCDEF01"

                elif (command == CA_cmd):
		        if line[3:len(line)] == 'FFFF':
                            resp = "OK"

                elif (command == PW_cmd):
                        if addr == '301':
                            if param == '10':
                                inj[0][1] = float(value)
                            if param == '11':
                                inj[0][2] = float(value)
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

