#!/usr/bin/python
import sqlite3
import time
import sys
import subprocess
#import syslog
import signal
import os

FLASH_DB = '/var/lib/accu4/accu4_db.sqlite'
stop_processes = 0
manual_ip = 0

#Argument Checking
if(len(sys.argv) >= 2) or (len(sys.argv) <= 4):
	if(sys.argv[1] == "a4m"):
		ip = "2"
		board_num = 1
	elif(sys.argv[1] == "a4b"):
		ip = "3"
		board_num = 2
	elif(sys.argv[1] == "a4i1"):
		ip = "4"
		board_num = 3
	elif(sys.argv[1] == "a4i2"):
		board_num = 4
		ip = "5"
	else:
		print "Unknown Board Type"
		sys.exit(0)
	if(len(sys.argv) == 3) or (len(sys.argv) == 4):
		if(sys.argv[2] == "stop"):
			stop_processes = 1
		elif(sys.argv[2] == "manual_ip"):
			manual_ip = 1
		else:
			print "Invalid Stop Flag: '%s'\n" % sys.argv[2]
			sys.exit(0)
#			syslog.syslog(syslog.LOG_ERR, "Incorrect Data Type")
else:
	print "Invalid Number of Arguments\nFormat is 'create_json_pipe.sh (Board)'\n"
	sys.exit(0)

#Kill Previous JSON Pipes
file_path=[]
file_path.append("/var/tmp/PIDS/" + sys.argv[1] + "/send_socat.pid")
file_path.append("/var/tmp/PIDS/" + sys.argv[1] + "/json_read.pid")
file_path.append("/var/tmp/PIDS/" + sys.argv[1] + "/json_hash.pid")
file_path.append("/var/tmp/PIDS/" + sys.argv[1] + "/socat.pid")

for i in range(4):

	if(os.path.isfile(file_path[i])):	
		file = open(file_path[i], 'r')
		PID = file.read()

		if(len(PID) > 0):
			try:
				os.kill(int(PID), signal.SIGTERM)
			except Exception, error:
				print error
		os.remove(file_path[i])

if(stop_processes == 0):
	#Connect to the SQLITE Database (Default Timeout is 5 Seconds)
	connect = sqlite3.connect(FLASH_DB)
	cursor = connect.cursor()

	#Start Socat with the Correct Arguments

	#Get the Board Set Number
	try:
		cursor.execute('SELECT a4b_available, a4i_available, board_set FROM load_arm_layout')
	except:
		#Default and Set Error
		result = [[0,0,0]]
	else:
		result = cursor.fetchall()

	#Check to Make Sure the Board Requested is Programmed
	boards_prog = result[0][0] + result[0][1] + 1
	print str(result[0][0]) + " " + str(board_num)
	if(board_num > boards_prog):	
		print sys.argv[1] + " Board Not Programmed\n"
		sys.exit(0)

	#Start the JSON Pipes
	command=[]
	process=[]
	command.append("send_socat.sh " + sys.argv[1])
	command.append("json_hash")
	if(manual_ip == 1):
		print "Manual IP Detected"
		command.append("socat - tcp:" + sys.argv[3] + ":6078")
	else:
		command.append("socat - tcp:10.0." + str(result[0][2]) + "." + ip + ":6078")
	command.append("json_read &")

	for j in range(4):
		try:
			if(j == 0):
				process.append(subprocess.Popen(command[j].split(), stdout=subprocess.PIPE))
			elif(j == 3):
				null_file = open(os.devnull, 'w')
				process.append(subprocess.Popen(command[j].split(), stdin=process[j-1].stdout, stdout=null_file))
			else:
				process.append(subprocess.Popen(command[j].split(), stdin=process[j-1].stdout, stdout=subprocess.PIPE))
			
		except Exception, error:
			print "Failed to Start " + command[j] + ":"
			print error
		#	syslog.syslog(syslog.LOG_ERR, "Failed to Start " + command[j] + ":")
		
		file = open(file_path[j], "w")
		file.write("%i" % process[j].pid)
		file.close()

		time.sleep(1)
		process[j].poll()

		#If One Process Fails, Terminate the Rest
		if (process[j].returncode is None):
			continue
		else:
			for k in range(j + 1):
				file = open(file_path[k], 'r')
				PID = file.read()
				if(len(PID) > 0):
					try:
						os.kill(int(PID), signal.SIGTERM)
					except Exception, error:
						print error
	
				os.remove(file_path[k])
				exit(1)

if(stop_processes == 0):
	print "Start the JSON Pipe for the " + sys.argv[1]
else:
	print "Stop the JSON Pipe for the " + sys.argv[1]
