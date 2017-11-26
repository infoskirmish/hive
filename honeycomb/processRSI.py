#!/usr/bin/python

from xml.dom.minidom import parseString
import os
import re
import time

#Debugging Function
DEBUG = True
#DEBUG = False

def debug(msg=" "):
	if DEBUG:
		print msg

def logEntry( processedFileName, resultDictionary ):

	#Always create and open a file named "preprocessedBeacon.log" for logging preprocessed File results 
	#Call this function before all returns...
	logFile = open( "beacon_logs/preprocessedBeacon.log", 'a')
	logEntry= "(" + time.asctime( time.localtime(time.time()) ) + ")    " + processedFileName + ":     " + str(resultDictionary) + "\n"
	logFile.write( logEntry )
	logFile.close
	return	


#Class to keep interface information...
class Interface:
	"Store interface parameters"
	def __init__(self, name, ipv4_Address, macAddress, ipv6_Address):
		self.name = name
		self.ipv4_Address = ipv4_Address
		if macAddress:
			self.macAddress = macAddress.lower()
		else:
			self.macAddress = macAddress
		self.ipv6_Address = ipv6_Address

	def getName(self):
		return self.name

	def getMacAddress(self):
		return self.macAddress

	def getIpv4Address(self):
		return self.ipv4_Address

	def getIpv6Address(self):
		return self.ipv6_Address

	def __str__(self):
		return "%s: %s %s %s" %(self.name, self.ipv4_Address, self.macAddress, self.ipv6_Address)

def preprocessFile( inputFile ):

	#Return a dictionary used to postprocess the file after going through it originally
	#bb_IP is the original bb_IP Address
	#vps_IP is the original source IP Address
	#newIP is the calculated contact IP address via the gateway
	#error is the last error that occured before exiting...
	dict={ }
	dict['bb_IP']= None	
	dict['vps_IP']= None	
	dict['newIP']= None	
	dict['error']= None

	#open the xml like file for processing
	#file =  open ('testinput.rsi', 'r')
	file = open( inputFile, 'r')

	#convert to string
	data = file.read()
	#close file because we dont need it anymore
	file.close()
	command="/bin/cp " + inputFile + " beacons/"

	print command
	os.system(command)

	dom=parseString(data)

	#retrieve IP tag data
	bb_IP = dom.getElementsByTagName('IP')[0].toxml().replace('<IP>','').replace('</IP>','')
	dict['bb_IP']= str (bb_IP)

	#retrieve  addressString tag data
	vps_IP = dom.getElementsByTagName('addressString')[0].toxml().replace('<addressString>','').replace('</addressString>','')
	dict['vps_IP']= str(vps_IP) 

	#retrieve the MACAddress xml tag (<MACAddress>data</MACAddress>) that the parse finds with the name <MACAddress>
	macTag = dom.getElementsByTagName('MACAddress')[0].toxml().replace('<MACAddress>','').replace('</MACAddress>',' ')

	checkForExtraData = dom.getElementsByTagName('extraData')
	if not checkForExtraData:
		dict['error']= "No extraData tag, hive version is too old (i.e. prior 2.4)"
		#debug( dict )
		logEntry( inputFile, dict )
		return dict 

	#retrive hiveVersion string
	hiveVersion = dom.getElementsByTagName('extraData')[1].toxml().replace('<extraData label="hiveVersion">',' ').replace('</extraData>','')

	try:
			float(hiveVersion)
	except ValueError:
		dict['error']= "No extraData tag with hiveVersion, hive version is too old (i.e. prior 2.4)"
		#debug( dict )
		logEntry( inputFile, dict )
		return dict 

	#retrive os tag string
	osType = dom.getElementsByTagName('extraData')[2].toxml().replace('<extraData label="os">',' ').replace('</extraData>','')

	debug()
	debug( "OS: %s                       hiveVersion: %s               ReportedMAC: %s" %(osType, hiveVersion, macTag) )
	debug()
	debug( "bb_IP= %s                                                      vps_IP= %s"    %(bb_IP, vps_IP) )
	xml_IFCONFIG_Tag = dom.getElementsByTagName('extraData')[4].toxml()
	interfaceList = [ ]
	if 'ikro' in osType:
		#Mikrotik Beacons
		interface = None
		ip = None
		mac = None
		ip6 = None
		interfaceAdded = 0
		for line in xml_IFCONFIG_Tag.split('\n'): 
			if ' &lt;' in line:
				interfaceLine=str(line).split(': &lt;')[0]
				#debug( str(interfaceLine) )
				if 'inet6 addr' not in line:
					if interface:
						#Save last found interface details
						newInterface = Interface(interface, ip, mac, ip6)
						interfaceList.append(newInterface)
						interface = None
						mac = None
						ip = None
						ip6 = None

					#New interface so interfaceAdded with new name
					interfaceAdded = 1
					interface = str(line).split(':')[0]
					#debug( "interfaceName= %s" %(interface))
			
				else:
					#Save ipv6 field...
					ip6 = str(line).split()[2]
					#debug( "ipv6_Address = %s" %(ip6))
					if interfaceAdded:
						#Save last found interface details
						newInterface = Interface(interface, ip, mac, ip6)
						interfaceList.append(newInterface)
						interfaceAdded = 0
						interface = None
						mac = None
						ip = None
						ip6 = None

			if 'HW addr:\t' in line:
				mac = str(line).split()[2]
				#debug( "macAddress= %s" %(mac))
	
			if 'inet addr:\t' in line:
				ip = str(line).split()[2]
				#debug( "ipAddress= %s" %(ip))

	elif 'olar' in osType:
		#Solaris Beacons
		interface = None
		ip = None
		mac = None
		ip6 = None
		interfaceAdded = 0
		for line in xml_IFCONFIG_Tag.split('\n'):
			if '&lt;' in line:
				interfaceAdded = 1
				if interface:
					newInterface = Interface(interface, ip, mac, ip6)
					interfaceList.append(newInterface)

				interface = None
				interface = str(line).split(':')[0]
				#debug( "interfaceName= %s" %(interface))
				ip = None
				mac = None
				ip6 = None

			if 'inet6' in line:
				ip6 = str(line).split()[1]
				#debug( "ipv6_Address= %s" %(ip6))
			elif 'inet' in line:
				ip =  str(line).split()[1]
                #debug( "ipv4_Address= %s" %(ip))

			if 'ether' in line:
				mac = str(line).split()[1]
				#debug( "macAddress= %s" %(mac))
				if interfaceAdded == 1:
					newInterface = Interface(interface, ip, mac, None)
					interfaceList.append(newInterface)
					interfaceAdded = 0
					interface = None
	else:
		#Linux Beacons
		interface = None
		ip = None
		mac = None
		ip6 = None
		interfaceAdded = 0
		for line in xml_IFCONFIG_Tag.split('\n'):
			if 'Link encap' in line:
				interfaceAdded = 1
				if interface:
					newInterface = Interface(interface, ip, mac, ip6)
					interfaceList.append(newInterface)

				interface = None
				interface = str(line).split()[0]
				#debug( "interfaceName= %s" %(interface) )
				ip = None
				mac = None
				ip6 = None

			if 'inet addr' in line:
				ip = str(line).split()[1].replace('addr:','')
				#debug( "ipAddress= %s" %(ip))

			if 'HWaddr' in line:
				mac = str(line).split()[4]
				#debug( "macAddress= %s" %(mac))

			if 'inet6 addr:' in line:
				ip6 = str(line).split()[2].replace('addr:','')
				#debug( "ipAddress= %s" %(ip6))
				newInterface = Interface(interface, ip, mac, ip6)
				interfaceList.append(newInterface)
				interfaceAdded = 0
				interface = None

	if interfaceAdded == 1:
		if interface:
			newInterface = Interface(interface, ip, mac, ip6)
			interfaceList.append(newInterface)

	debug()
	debug( "Interface List:" )
	for eachInterface in interfaceList:
		debug( eachInterface )
	debug()

	gatewayList=[]
	xml_NETSTATRN_Tag = dom.getElementsByTagName('extraData')[5].toxml()
	if 'olar' in osType:
		for line in xml_NETSTATRN_Tag.split('\n'):
			p = re.compile(r'default')
			iterator = p.finditer(line)
			for match in iterator:
				if match.start() == 0:
					if 'UG' in line:
						gatewayLine=str(line).split()
						debug( "Internal Gateway Address= " + gatewayLine[1] )
						gatewayList.append(gatewayLine[1])
	elif 'ikro' in osType:
		for line in xml_NETSTATRN_Tag.split('\n'):
			p = re.compile(r'0.0.0.0')
			iterator = p.finditer(line)
			for match in iterator:
				if match.start() == 1:
					if 'UG' in line:
						gatewayLine=str(line).split()
						debug( "Internal Gateway Address= " + gatewayLine[1] + ", netmask:" +gatewayLine[2])
						gatewayList.append(gatewayLine[1])
	else:
		for line in xml_NETSTATRN_Tag.split('\n'):
			p = re.compile(r'0.0.0.0')
			iterator = p.finditer(line)
			for match in iterator:
				if match.start() == 0:
					if 'UG' in line:
						gatewayLine=str(line).split()
						#debug( "Internal Gateway Address= " + gatewayLine[1] + ", netmask:" +gatewayLine[2])
						gatewayList.append(gatewayLine[1])

	if len(gatewayList) == 0:
		dict['error'] = "No gateway was found."
		#debug( dict )
		logEntry( inputFile, dict )
		return dict 

	externalIP=[]
	for eachGateway in gatewayList:
		bestmatch=gatewayLine[1].split('.')
		bestPossibleMatch=bestmatch[0]+"."+bestmatch[1]+"."+bestmatch[2]+"."+bestmatch[3]
		secondBestMatch=bestmatch[0]+"."+bestmatch[1]+"."+bestmatch[2]
		thirdBestMatch=bestmatch[0]+"."+bestmatch[1]
		lastMatch=bestmatch[0]
		#debug( "Matching order follows: " +bestPossibleMatch + ", " + secondBestMatch + ", " + thirdBestMatch + ", " + lastMatch )

		for eachInterface in interfaceList:
			if eachInterface.getIpv4Address():
				#debug( "Checking " + eachInterface.getIpv4Address())
				if bestPossibleMatch in eachInterface.getIpv4Address():
					#debug( "matched %s" %(eachInterface.getIpv4Address()) )
					externalIP.append(eachInterface.getIpv4Address())
				elif secondBestMatch in eachInterface.getIpv4Address():
					#debug( "matched %s" %(eachInterface.getIpv4Address()) )
					externalIP.append(eachInterface.getIpv4Address())
				elif thirdBestMatch in eachInterface.getIpv4Address():
					#debug( "matched %s" %(eachInterface.getIpv4Address()) )
					externalIP.append(eachInterface.getIpv4Address())
				elif lastMatch in eachInterface.getIpv4Address():
					#debug( "matched %s" %(eachInterface.getIpv4Address()) )
					externalIP.append(eachInterface.getIpv4Address())

	if externalIP:
		debug( 'calculated_IP= ' + str(externalIP[0]) )
		debug( )
		dict['newIP']=str(externalIP[0])
	
	#debug( dict )
	logEntry( inputFile, dict )
	return dict

def postProcessFile( inputFile, goodBeaconDir, badBeaconDir, preProcessingResults ):

	if preProcessingResults['error'] != None:
		if badBeaconDir[len(badBeaconDir)-1] == '/':
			command="/bin/mv " + inputFile + " " + badBeaconDir
		else:
			command="/bin/mv " + inputFile + " " + badBeaconDir +'/ '

		#print command
		os.system(command)
		
	else:

		#debug( "No error" )

		#open the xml like file for processing
		infile = open( inputFile, 'r')

		#convert to string
		data = infile.read()
		#close file because we dont need it anymore
		infile.close()

		#open new outputFile in goodBeaconDir
		head, tail = os.path.split(inputFile)
		if goodBeaconDir[len(goodBeaconDir)-1] == '/':
			outfile = open( goodBeaconDir+tail, 'w' )
		else:
			outfile = open( goodBeaconDir + '/' + tail, 'w')

		#Parse data
		print data
		dom=parseString(data)

		#retrieve all BeaconData 
		beaconData = dom.getElementsByTagName('ToolHandlerFile')[0].toxml()

		for line in beaconData.split('\n'):
			if '<IP>' in line:
				oldIp = preProcessingResults['bb_IP']
				nIp = preProcessingResults['vps_IP']
				if nIp == '10.177.76.14':
					nIp = '82.221.131.100'
				elif nIp == '10.177.76.18':
					nIp = '78.138.97.145'
				elif nIp == '10.177.76.22':
					nIp = '192.99.0.128'
				elif nIp == '10.177.76.26':
					nIp = '201.218.252.110'
				elif nIp == '10.177.76.30':
					nIp = '186.193.44.130'
				elif nIp == '10.177.77.34':
					nIp = '190.120.236.211'
				elif nIp == '10.177.77.38':
					nIp = '193.34.145.82'
				elif nIp == '10.177.77.42':
					nIp = '31.210.100.208'
				elif nIp == '10.177.77.46':
					nIp = '103.8.24.143'
				elif nIp == '10.177.77.50':
					nIp = '46.108.130.10'
				ipLine = line.replace( oldIp, nIp)
				#print ipLine
				outfile.write(ipLine+'\n')

			elif '<addressString' in line and preProcessingResults['newIP'] != None: 
				#print "In addressString, line=" + line
				oldvps = preProcessingResults['vps_IP']
				#print "Old addressString = " + oldvps
				newip = preProcessingResults['newIP']
				#print "New addressString = " + newip
				newLine = line.replace( oldvps, newip)
				#print "New addressString line=" + newLine
				outfile.write(newLine+'\n')
			
			else:
				outfile.write(line+'\n')
					
		outfile.close

		command="/bin/rm " + inputFile
#		command="/bin/mv " + inputFile + " orig_beacons/"
#
#		print command
		os.system(command)

def processRSIFile( newFileName ):
	import sys, os
	results=preprocessFile( newFileName )
#	print 
#	print results
#	print
	postProcessFile( newFileName, "beacons/", "e_beacons/", results )

if __name__ == "__main__":
	import sys, os
	processRSIFile( sys.argv[1] )

