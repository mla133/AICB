#!/usr/bin/env python

import os, sys

if (len(sys.argv) < 2):
    print "Argument needed: a4m or a4b"
    exit()

if (sys.argv[1] == 'a4m'):
    fifo_path = '/var/tmp/a4m/socat_output_mtr_inj_fifo' #name of FIFO pipe
    data_path = '/dev/shm/a4m_mtr_inj_input_data_file' #name of DATA file

elif (sys.argv[1] == 'a4b'):
    fifo_path = '/var/tmp/a4b/socat_output_mtr_inj_fifo' #name of FIFO pipe
    data_path = '/dev/shm/a4b_mtr_inj_input_data_file' #name of DATA file
else:
    print "Argument needed: a4m or a4b"
    exit()

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
inj_addr = 0

#         0   ,      1,   2,   3,   4, 5,        6, 7, 8, 9,10,11,12,   13,  14,15,16,17
inj =  [
        # A4M
        ['301', 5000.0, 1.0, 0.0, 0.0, 0, 3785.412, 0, 0, 2, 0, 0, 0, 25.0, 0.0, 0, 0, 0],
        ['302', 5000.0, 1.0, 0.0, 0.0, 0, 3785.412, 0, 0, 2, 0, 0, 0, 25.0, 0.0, 0, 0, 0],
        ['303', 5000.0, 1.0, 0.0, 0.0, 0, 3785.412, 0, 0, 2, 0, 0, 0, 25.0, 0.0, 0, 0, 0],
        ['304', 5000.0, 1.0, 0.0, 0.0, 0, 3785.412, 0, 0, 2, 0, 0, 0, 25.0, 0.0, 0, 0, 0],
        ['305', 5000.0, 1.0, 0.0, 0.0, 0, 3785.412, 0, 0, 2, 0, 0, 0, 25.0, 0.0, 0, 0, 0],
        ['306', 5000.0, 1.0, 0.0, 0.0, 0, 3785.412, 0, 0, 2, 0, 0, 0, 25.0, 0.0, 0, 0, 0],
        ['307', 5000.0, 1.0, 0.0, 0.0, 0, 3785.412, 0, 0, 2, 0, 0, 0, 25.0, 0.0, 0, 0, 0],
        ['308', 5000.0, 1.0, 0.0, 0.0, 0, 3785.412, 0, 0, 2, 0, 0, 0, 25.0, 0.0, 0, 0, 0],
        ['309', 5000.0, 1.0, 0.0, 0.0, 0, 3785.412, 0, 0, 2, 0, 0, 0, 25.0, 0.0, 0, 0, 0],
        ['310', 5000.0, 1.0, 0.0, 0.0, 0, 3785.412, 0, 0, 2, 0, 0, 0, 25.0, 0.0, 0, 0, 0],
        # A4B
        ['401', 5000.0, 1.0, 0.0, 0.0, 0, 3785.412, 0, 0, 2, 0, 0, 0, 25.0, 0.0, 0, 0, 0],
        ['402', 5000.0, 1.0, 0.0, 0.0, 0, 3785.412, 0, 0, 2, 0, 0, 0, 25.0, 0.0, 0, 0, 0],
        ['403', 5000.0, 1.0, 0.0, 0.0, 0, 3785.412, 0, 0, 2, 0, 0, 0, 25.0, 0.0, 0, 0, 0],
        ['404', 5000.0, 1.0, 0.0, 0.0, 0, 3785.412, 0, 0, 2, 0, 0, 0, 25.0, 0.0, 0, 0, 0],
        ['405', 5000.0, 1.0, 0.0, 0.0, 0, 3785.412, 0, 0, 2, 0, 0, 0, 25.0, 0.0, 0, 0, 0],
        ['406', 5000.0, 1.0, 0.0, 0.0, 0, 3785.412, 0, 0, 2, 0, 0, 0, 25.0, 0.0, 0, 0, 0],
        ['407', 5000.0, 1.0, 0.0, 0.0, 0, 3785.412, 0, 0, 2, 0, 0, 0, 25.0, 0.0, 0, 0, 0],
        ['408', 5000.0, 1.0, 0.0, 0.0, 0, 3785.412, 0, 0, 2, 0, 0, 0, 25.0, 0.0, 0, 0, 0],
        ['409', 5000.0, 1.0, 0.0, 0.0, 0, 3785.412, 0, 0, 2, 0, 0, 0, 25.0, 0.0, 0, 0, 0],
        ['410', 5000.0, 1.0, 0.0, 0.0, 0, 3785.412, 0, 0, 2, 0, 0, 0, 25.0, 0.0, 0, 0, 0],
       ]
 
while True:

	fifo = open(fifo_path, "r")
	for line in fifo:
		print "\x1b[32;40mAccuLoad:  " + line + "\x1b[0m"
		addr = line[0:3]
                command = line[3:5]
                param = line[6:8]
                value = line[8:len(line)]

                if (sys.argv[1] == 'a4m'):
                    inj_addr = (int)(addr)-301
                if (sys.argv[1] == 'a4b'):
                    inj_addr = (int)(addr)-401

		if (command == IN_cmd):
                        inj[inj_addr][14] += inj[inj_addr][13] / inj[inj_addr][6]
                        inj[inj_addr][15] += inj[inj_addr][14] * inj[inj_addr][1] 
			resp = "OK"

		elif (command == EP_cmd or command == DP_cmd):
                        resp = "OK"

                elif (command == OS_cmd):
                        if param == 'P':
                            resp = "OS P %d" % inj[inj_addr][17] 
                        if param == 'S':
                            resp = "OS S %d" % inj[inj_addr][16]
                            
                elif (command == SO_cmd):
                        if param == 'P':
                            inj[inj_addr][17] = int(value)
                        if param == 'S':
                            inj[inj_addr][16] = int(value)
                        resp = "OK"

		elif (command == TS_cmd):
                        resp = "TS %12.3f 0000" % (float)(inj[inj_addr][14])

		elif (command == ST_cmd):
			resp = "ST 0000"

		elif (command == SV_cmd):
			resp = "SV 06 ABCDEF01"

                elif (command == CA_cmd):
		        if line[3:len(line)] == 'FFFF':
                            resp = "OK"

                elif (command == PW_cmd):
                        if param == '10':
                            inj[inj_addr][1] = float(value)
                        if param == '11':
                            inj[inj_addr][2] = float(value)
                        if param == '20':
                            inj[inj_addr][3] = float(value)
                        if param == '20':
                            inj[inj_addr][4] = float(value)
                        if param == '23':
                            inj[inj_addr][6] = float(value)
                        if param == '24':
                            inj[inj_addr][7] = int(value)
                        if param == '25':
                            inj[inj_addr][8] = int(value)
                        if param == '26':
                            inj[inj_addr][9] = int(value)
                        if param == '30':
                            inj[inj_addr][13] = float(value)
                        resp = "OK" 

                elif (command == PC_cmd):
                        resp = "PC %06d" % (int)(inj[inj_addr][15])

                elif (command == RC_cmd):
                        inj[inj_addr][15] = 0
                        resp = "OK"

		else :
			resp = "NO00"

	print "\x1b[31;40mInjector:  " + addr + resp + "\x1b[0m"
	fifo.close()
	
	data = open(data_path, "w")
	data.write(addr + resp)
	data.close()

