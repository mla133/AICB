#!/usr/bin/python

import subprocess
import signal
import sys
import os
#import syslog

#Argument Checking
#syslog.openlog('create-json-write.py')

stop_processes = 0

if(len(sys.argv) == 4) or (len(sys.argv) == 5):
	if(sys.argv[1] == "data"):
		type = "d"
	elif(sys.argv[1] == "serial"):
		type = "s"
	elif(sys.argv[1] == "command"):
		type = "cmd"
	elif(sys.argv[1] == "mtr_inj"):
		type = "mi"
	else:
		print "Invalid Data Type: '%s'\n" % sys.argv[1]
		sys.exit(0)
		#syslog.syslog(syslog.LOG_ERR, "Incorrect Data Type")

	if(len(sys.argv) == 5):
		if(sys.argv[4] == "stop"):
			stop_processes = 1
		else:
			print "Invalid Stop Flag: '%s'\n" % sys.argv[1]
			sys.exit(0)
			#syslog.syslog(syslog.LOG_ERR, "Incorrect Data Type")

else:
	print "Invalid Number of Arguements\nFormat is 'create_json_write.sh (Data Type) (Board) (Instance) (Optional:Stop)'\n"
	sys.exit(0)
	#syslog.syslog(syslog.LOG_ERR, "Incorrect Number of Arguments")

#Kill Old Json_write processes
#Read the PID
if(sys.argv[1] == "serial"):
	file_path = "/var/tmp/PIDS/" + sys.argv[2] + "/" + sys.argv[1] + sys.argv[3] + "_json_write.pid"
else:
	file_path = "/var/tmp/PIDS/" + sys.argv[2] + "/" + sys.argv[1] + "_json_write.pid"
print file_path
if(os.path.isfile(file_path)):	
	file = open(file_path, 'r')
	PID = file.read()
	if(len(PID) > 0):
		try:
			os.kill(int(PID), signal.SIGTERM)
		except Exception:
			print "No Process"

	os.remove(file_path)

if(stop_processes == 0):
	command = "json_write %s %s %s" % (sys.argv[1], sys.argv[2], sys.argv[3])
	try:
		json_write_process = subprocess.Popen(command.split(), stdout=subprocess.PIPE)
	except Exception:
		print "Failed to Start json_write"
		#syslog.syslog(syslog.LOG_ERR, "Failed to Start json_write " + sys.argv[1] + " " + sys.argv[2] + " " + sys.argv[3])

	command = "write_file.sh %s %s %s &" % (type, sys.argv[2], sys.argv[3])
	try:
		#Redirect Output to Null
		null_file = open(os.devnull, 'w')
		write_file_process = subprocess.Popen(command.split(), stdin=json_write_process.stdout , stdout=null_file)
	except Exception, error:
		print "Failed to Start write_file.sh\n"
		print error
		json_write_process.kill()
		#syslog.syslog(syslog.LOG_ERR, "Failed to Start json_write " + sys.argv[1] + " " + sys.argv[2] + " " + sys.argv[3])

	file = open(file_path, "w")
	file.write("%i" % json_write_process.pid)
	print "Started Instance " + sys.argv[3] + " of " + sys.argv[1] + " for the " + sys.argv[2]
else:
	print "Stopped Instance " + sys.argv[3] + " of " + sys.argv[1] + " for the " + sys.argv[2]	


