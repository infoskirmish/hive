#!/usr/bin/python

# module import

from time import sleep
import shlex
import optparse
import fileinput
import getpass
import platform
import random
import os, sys
import re
import time
import socket
import base64
import logging



if sys.version >= '3' and sys.version < '4':
   import urllib.request, urllib.parse, urllib.error, urllib.request, urllib.error, urllib.parse
   import hashlib
   import http.client
   import http.cookiejar
   import email.generator
   pyver = 3
elif sys.version < '3':
   import urllib, urllib2
   import md5
   import httplib
   import cookielib
   import mimetools
   pyver = 2
else:
   print("This version of python is not supported!")
   sys.exit(-1)

#Description: Python script to upload files to the SWIFT system using LWP
#             This is designed to work for Python >2.6 and 3.x
#Filename: swift_upload_univ.py

version = '20120627.univ'

####### Return Code Documentation ###
##### 
##### This code will return a specific code for various exit conditions. 
#####
##### 0 = Successful transfer
##### 1 = A transfer has failed
##### 2 = Status Timeout 
##### 3 = Configuration Error
##### 4 = Invalid File List
##### 5 = Invalid Login Information
##### 9 = Server Error
##### 10 = Could not run headless through python module call
##### 99 = Successful noop run
#####
#####################################


################# user must set these variables
feederip = ''
feederarray = []
############# end user must set these variables

headless = 0
last_index = -1
feeder_index = 0
feeder_counter = 0
error_string = ''

#default_feeder_ip = '172.28.5.51' #Admin
#default_feeder_ip = '192.168.230.11' #BigMac
default_feeder_ip = '172.20.17.51' #Icon
#default_feeder_ip = '172.24.2.41' #TDN
#default_feeder_ip = '10.249.91.60' #O
#default_feeder_ip = '192.168.254.9' #BAR


class swiftuploadException(Exception): pass


log = logging.getLogger('swift_upload_univ_py')


# legacy function call to comply with older OWT client -- returns non-zero when there's a problem
# pass a loggername if you want output to integrate with your existing logging infrastructure
def upload(what, username, password, description, where, IP_Address=default_feeder_ip, cutdir="0", loggername="swift_upload_univ_py"):
   # hardcoded other settings
   verbose = 0
   noop = 0
   debug = 0

   global headless
   global log

   headless = 1

   if isinstance(loggername, logging.Logger):
      log = loggername
   else:
      log = logging.getLogger(loggername)


   if what is None or username is None or password is None or where is None:
      log.error("Required arguments for upload() not given!!")
      return -1

   if description is None:
      description = "swift_upload_univ_py"

   return main("--username=" + username + " --password='" + password + "' --feederarray='" + IP_Address + "' --where=" + where + " --description='" + description + "' --verbose=" + str(verbose) + " --debug=" + str(debug) + " --noop=" + str(noop) + " --cutdir=" + str(cutdir) + " " + what )


# another uploader caller function that raises exceptions upon failure
# pass a loggername if you want output to integrate with your existing logging infrastructure
def uploader(files, user, pw, dest, feeder=default_feeder_ip, desc="swift_upload_univ_py", verbose="0", debug="0", noop="0", cutdir="0",loggername="swift_upload_univ_py"):
   # caller to import swift_upload.py as a module and run within
   # other python scripts   
   # NOTE: This is BETA!!  This has problems if you give it a bad feeder ip -- exception handling also not fully tested
   global headless
   global log

   headless = 1
   if isinstance(loggername, logging.Logger):
      log = loggername
   else:
      log = logging.getLogger(loggername)

   if files is None or user is None or pw is None or dest is None:
      log.error("Required arguments for uploader() not given!!")
      return -1

   # files should be a space-delimited list of files and folders (like from the command line)
   # current directory will be whatever the current directory is in the calling func
   #try:
   retval = main("--username=" + user + " --password='" + pw + "' --feederarray='" + feeder + "' --where=" + dest + " --description='" + desc + "' --verbose=" + str(verbose) + " --debug=" + str(debug) + " --noop=" + str(noop) + " --cutdir=" + str(cutdir) + " " + files )
   #except Exception, e:
   #   print "ERROR: " + e.code + " : " + e.msg + "\nContact Support"

   if retval == 0 or retval == 99:
      return 0
   elif retval == 1:
      raise swiftuploadException(time.asctime() + " : SWIFT error code: " + str(retval) + " : A transfer has failed")
   elif retval == 2:
      raise swiftuploadException(time.asctime() + " : SWIFT error code: " + str(retval) + " : Status timeout")
   elif retval == 3:
      raise swiftuploadException(time.asctime() + " : SWIFT error code: " + str(retval) + " : Configuration error")
   elif retval == 4:
      raise swiftuploadException(time.asctime() + " : SWIFT error code: " + str(retval) + " : Invalid file list")
   elif retval == 5:
      raise swiftuploadException(time.asctime() + " : SWIFT error code: " + str(retval) + " : Invalid login information")
   elif retval == 9:
      raise swiftuploadException(time.asctime() + " : SWIFT error code: " + str(retval) + " : Server error")
   elif retval == 10:
      raise swiftuploadException(time.asctime() + " : SWIFT error code: " + str(retval) + " : Error running headless through python module call")
   else:
      raise swiftuploadException(time.asctime() + " : SWIFT error code: " + str(retval) + " : Unknown error")

#end def uploader()

def main(argv=None):
   # globals
   global last_index
   global feeder_index
   global feeder_counter
   global error_string
   global feederarray
   global debug
   global OS
   global headless

   if argv is None:
      runtimeargs = sys.argv[1:]
      ch = logging.StreamHandler(sys.stdout) ### to output log to console (for interactive sessions)
      ch.setLevel(logging.INFO)
      log.addHandler(ch)
   else:
      runtimeargs = shlex.split(argv)
      headless = 1

   log.setLevel(logging.INFO)

   verbose = 0
   debug = 0
   sequence = 0
   configfile = ''
   username = ''
   password = ''
   where = ''
   description = ''
   baseurl = ''
   feeders = ''
   datamode = 'https'  ### temporarily set
   ua = ''
   req = ''
   loginmode = 'https'
   transfersize = 0
   zippedfiles = 0
   zipfilename = ''
   transfer_timestamp = ''
   loginurl = ''
   partitionurl = ''
   partitionsize = 32000000 ### Partition size is 32MB. Do NOT CHANGE without updating apache Servers.
   myconfig = {}
   postwhere = []
   squishyuser = 0
   htaccessuser = ''
   htaccesspass = ''
   feederpath = 'upload'  #### temporarily added for dev testing
   feederaddress = ''
   max_status_poll_sec = 3600  # Must be a multiple of status_poll_sec
   status_poll_sec = 10


   windows = 0

   os_detection()
   if OS == 'WINDOWS':
      windows = 1

   class MyParser(optparse.OptionParser):
      def format_description(self, formatter):
         return self.description

   parser = MyParser(version = version,
                     description = 
"""Version: %s 
NOTES:
This program will allow you to upload files or directories via SWIFT
Any arguments not set at run time will be asked for interactively.
Command line arguments override config file arguments.

 CONFIG FILE EXAMPLE:
    USERNAME=Script
    PASSWORD=Password
    DESCRIPTION=Test Description
    WHERE=CITYNAME
    FEEDERARRAY=default_feeder_ip,default_feeder_ip
    DATAMODE=http
""" % version)
   parser.add_option('--username', dest='username')
   parser.add_option('--password', dest='password')
   parser.add_option('--where', dest='where')
   parser.add_option('--description', dest='description', metavar='DESC')
   parser.add_option('--feederarray', dest='feeders', metavar='FEEDERS')
   parser.add_option('--configfile', dest='configfile', metavar='CONFIGFILE')
   parser.add_option('--verbose', dest='verbose', type='int', metavar='VALUE', default=0)
   parser.add_option('--baseurl', dest='baseurl', metavar='BASEURL')
   parser.add_option('--sequence', dest='sequence', metavar='SEQ')
   parser.add_option('--cutdir', dest='cutdir', type='int', metavar='NUM', default=0)
   parser.add_option('--debug', dest='debug', type='int', metavar='VALUE', default=0)
   parser.add_option('--noop', dest='noop', type='int', metavar='VALUE', default=0)

   (options, args) = parser.parse_args(runtimeargs)

   if options.verbose:
      squishyuser = options.verbose
      verbose = options.verbose
      
   if options.debug:
      debug = options.debug
      log.setLevel(logging.DEBUG)
      try:
         ch.setLevel(logging.DEBUG)
      except:
         pass

   noop = options.noop

   if not args:
      parser.error('No arguments given!')
      return 3
   else:
      # read in configuration from config file
      if options.configfile:
         for line in fileinput.input( options.configfile ):
            sline = line.split('=')
            if not len(sline) < 2:
               myconfig[sline[0]] = sline[1]
               if options.debug:
                  log.debug("DEBUG: Name = " + sline[0] + " and val =" + myconfig[sline[0]] + "=") 

      if options.cutdir:
         cutdir = options.cutdir
      else:
         cutdir = False;


      # if the baseurl wasn't set on the command line, look in the configuration hash, or IGNORE the parameter
      if options.baseurl:
         baseurl = options.baseurl
      else:
         if 'BASEURL' in myconfig:
            baseurl = myconfig['BASEURL']
      if not feederpath:
         if 'FEEDERPATH' in myconfig:
            feederpath = myconfig['FEEDERPATH']

      # If the feeder array wasn't set on the command line, look in the configuration hash, or prompt user for the information
      if options.feeders and not baseurl:
         feeders = options.feeders
      if not options.feeders and not baseurl:
         if 'FEEDERARRAY' in myconfig:
            feeders = myconfig['FEEDERARRAY']
         elif not headless:
            if pyver == 3:
               feeders = input('Feeder IP Address [' + default_feeder_ip + ']: ')
            else:
               feeders = raw_input('Feeder IP Address [' + default_feeder_ip + ']: ')
            if not feeders:
               feeders = default_feeder_ip
            squishyuser = 1
         else:
            return 10
      # Feeder IP address array creation
      feederarray = feeders.split(',')

      # If the data mode wasn't set on the command line, look in the configuration hash, or prompt user for the information
      if not datamode:
         if 'DATAMODE' in myconfig:
            datamode = myconfig['DATAMODE']
         else:
            # Assume http for data mode
            datamode = 'http'
      if not htaccessuser:
         if 'HTACCESSUSER' in myconfig:
            htaccessuser = myconfig['HTACCESSUSER']
      if not htaccesspass:
         if 'HTACCESSPASS' in myconfig:
            htaccessuser = myconfig['HTACCESSPASS']

      # Check to see if the baseurl parameter was set to override the feeder ip
      if baseurl:
         loginurl = baseurl + "/index.php"
         partitionurl = baseurl + "/partition.php"
      else:
         feederip = new_feeder_address()

         if htaccessuser and htaccesspass:
            feederaddress = htaccessuser + ":" + htaccesspass + "@" + feederip
         else:
            feederaddress = feederip
         loginurl = loginmode + "://" + feederaddress + "/" + feederpath + "/index.php" ### Login URL
         partitionurl = loginmode + "://" + feederaddress + "/" + feederpath + "/partition.php" ### Code that accepts uploads
      log.debug("DEBUG: Feeder IP: " + feederip + " Login URL: " + loginurl)

      # If the username wasn't set on the command line, look in the configuration hash, or prompt user for the information
      if options.username:
         username = options.username
      else:
         if 'USERNAME' in myconfig:
            username = myconfig['USERNAME']
         elif not headless:
            if pyver == 3:
               username = input('Username: ')
            else:
               username = raw_input('Username: ')
            squishyuser = 1
         else:
            return 10
      # If the password wasn't set on the command line, look in the configuration hash, or prompt user for the information
      if options.password:
         password = options.password
      else:
         if 'PASSWORD' in myconfig:
            password = myconfig['PASSWORD']
         elif not headless:
            password = getpass.getpass('Password: ')
            squishyuser = 1
         else:
            return 10
      if squishyuser:
         log.info("\n\nINFO: Logging in to feeder " + feederip)

      ##### Create Useragent for connecting to the server side
#      $ua = LWP::UserAgent->new;
#      $ua->agent("SWIFT-Upload/4.2");  build an opener
#      $ua->cookie_jar({});     created an empty cookiejar 
#      $ua->timeout("15");      for post-2.6 use urllib2.urlopen(url, data, timeout), otherwise socket timeout
#      eval { $ua->ssl_opts(verify_hostname=>0) };    done by default with urllib2

      default_timeout = 180 

      socket.setdefaulttimeout(default_timeout)

      if pyver == 3:
         cj = http.cookiejar.CookieJar()
         opener = urllib.request.build_opener(urllib.request.HTTPCookieProcessor(cj))
      else:
         cj = cookielib.CookieJar()
         opener = urllib2.build_opener(urllib2.HTTPCookieProcessor(cj))

      opener.addheaders = [('User-agent', 'SWIFT-Upload/4.2')]

      if pyver == 3:
         urllib.request.install_opener(opener)
      else:
         urllib2.install_opener(opener)
      ##### login to the API, expect result as a list of wherefields and the transfer ID number

      successful_login = 0
      login_count = 0
      failed_login_count = 0

      loginresponse = ''

      while not successful_login:
         values = [('loginusername', username),
                   ('loginpassword', password),
                   ('client', 'script'),
                   ('Button', 'Login') ]
         if pyver == 3:
            reqdata = urllib.parse.urlencode(values).encode("utf-8")
            req = urllib.request.Request(loginurl, reqdata)
         else:
            reqdata = urllib.urlencode(values)
            req = urllib2.Request(loginurl, reqdata)

         if pyver == 3: 
            try:
               resp = urllib.request.urlopen(req)
            except urllib.error.URLError, e:
               log.warn("WARNING: " + feederip + " Server Unavailable (URLError)")
               login_count = login_count + 1
            except urllib.error.HTTPError, e:
               if e.code == '404':
                  log.warn("WARNING: " + feederip + " Server Unavailable (HTTPError)")
               if e.code == '500' and loginmode == 'https':
                  log.warn("WARNING: " + "Check that SSL Support enabled: " + feederip)
               login_count = login_count + 1
            except Exception, e: 
               log.debug("DEBUG: " + e)
               log.error("ERROR: General error\nContact Support")
               failed_login_count = failed_login_count + 1
               sleep(failed_login_count**2)
            else:
               loginresponse = resp.read()
               log.debug("DEBUG: Login Result: " + loginresponse.decode("utf-8"))
               if 'TRANSFERNAME' in loginresponse.decode("utf-8"):
                  successful_login = 1
               elif 'AUTH FAILED' in loginresponse.decode("utf-8"):
                  log.error("ERROR: Incorrect Login Information")
                  return 5
               elif 'Load is too high' in loginresponse.decode("utf-8") or 'Disk usage is too LOW' in loginresponse.decode("utf-8"):
                  login_count = login_count + 1
                  sleep(login_count**2)
         else:
            try:
               resp = urllib2.urlopen(req)
            except urllib2.URLError, e:
               log.warn("WARNING: " + feederip + " Server Unavailable (URLError)")
               login_count = login_count + 1
            except urllib2.HTTPError, e:
               if e.code == '404':
                  log.warn("WARNING: " + feederip + " Server Unavailable (HTTPError)")
               if e.code == '500' and loginmode == 'https':
                  log.warn("WARNING: " + "Check that SSL Support enabled: " + feederip)
               login_count = login_count + 1
            except Exception, e: 
               log.debug("DEBUG: " + e)
               log.error("ERROR: General error\nContact Support")
               failed_login_count = failed_login_count + 1
               sleep(failed_login_count**2)
            else:
               loginresponse = resp.read()
               log.debug("DEBUG: Login Result: " + loginresponse)
               if 'TRANSFERNAME' in loginresponse:
                  successful_login = 1
               elif 'AUTH FAILED' in loginresponse:
                  log.error("ERROR: Incorrect Login Information")
                  return 5
               elif 'Load is too high' in loginresponse or 'Disk usage is too LOW' in loginresponse:
                  login_count = login_count + 1
                  sleep(login_count**2)


         if failed_login_count > 10 or login_count > 10:
            log.error("ERROR: Multiple login failures, exiting")
            return 6

      ##### Get Transfer timestamp #####
      if pyver == 3:
         regexsearch = re.search('TRANSFERNAME\s([0-9]{16})==.*WHEREFIELDS\s(.*)==', loginresponse.decode("utf-8"))
      else:
         regexsearch = re.search('TRANSFERNAME\s([0-9]{16})==.*WHEREFIELDS\s(.*)==', loginresponse)
      transfer_timestamp = regexsearch.group(1)
      wherefields = regexsearch.group(2)
      error_string = error_string + "Transfer Timestamp = " + transfer_timestamp + "\n"
      log.debug("DEBUG: Transfer Timestamp = " + transfer_timestamp)

      ##### Calculate authorized wherefields

      wheres = wherefields.split(',')      
      validwherefields = {}

      for location in wheres:
         temp = location.split(':')
         levelANDdest = temp[1] + ":" + temp[2] #### Hacked to fix change made on 03-02-2011 to support destination levels for wherefields, gluing the two values back together before storing
         validwherefields[temp[0]] = levelANDdest

      ### Print validwherefields if debugging is enabled
      for i in validwherefields:
         log.debug("DEBUG: Authorized Wherefield: " + i + ", Level: " + validwherefields[i])

      validwhereselected = 0
      cmdline_where_good = 1
      while not validwhereselected:
         # If the list of where fields wasn't set on the command line, look in the configuration hash, or prompt user for the information
         if options.where and cmdline_where_good == 1:
            where = options.where
         else:
            if 'WHERE' in myconfig:
               where = myconfig['WHERE']
            elif not headless:
               if pyver == 3:
                  where = input('What project to save data to? [OAKLAND]: ')
               else:
                  where = raw_input('What project to save data to? [OAKLAND]: ')
               if not where:
                  where = 'OAKLAND'
            else:
               return 10
            squishyuser = 1

         ##### make the user input for the wherefields uppercase, since we always pass back uppercase information
         where = where.upper()

         ##### Compare the user input to the authorization list.
         requestedwheres = where.split(',')
         log.debug("DEBUG: " + str(requestedwheres) + " requested wherefields")

         for wherefield in requestedwheres:
            # check to see if requested field, exists in the authorized wherefields hash

            if wherefield in validwherefields:

               log.debug("DEBUG: Requested Wherefield: " + wherefield + ", Level: " + validwherefields[wherefield])

               postwhere.append(wherefield + ':' + validwherefields[wherefield])
               validwhereselected = 1
               cmdline_where_good = 1
            else:
               log.error("ERROR: Unauthorized or non-existent wherefield: " + wherefield + "\nValid Options:")
               for i in validwherefields:
                  log.error(i)
               log.error('')
               validwhereselected = 0
               cmdline_where_good = 0
               where = ''

      # If the description wasn't set on the command line, look in the configuration hash, or prompt user for the information
      if options.description:
         description = options.description
      else:
         if 'DESCRIPTION' in myconfig:
            description = myconfig['DESCRIPTION']
         elif not headless:
            if pyver == 3:
               description = input('Description: ')
            else:
               description = raw_input('Description: ')
            squishyuser = 1
         else:
            return 10

      # If the sequencing wasn't set on the command line, look in the configuration hash, or assume unsequenced
      if options.sequence:
         sequence = options.sequence
      else:
         if 'SEQUENCE' in myconfig:
            sequence = myconfig['SEQUENCE']
         else:
            ## if the user doesn't ask for it, we will assume unsequenced
            sequence = 0               

      # Print out variable values if debugging is on
      log.debug("DEBUG: Username = " + username)
      log.debug("DEBUG: Password = not displayed")
      log.debug("DEBUG: Where = " + where)
      log.debug("DEBUG: Description = " + description)
      log.debug("DEBUG: Detected Running OS: " + OS)
      log.debug("DEBUG: Sequencing: " + str(sequence)) 
      log.debug("DEBUG: Feeder IP Array: " + feeders)
      log.debug("DEBUG: Data Mode: " + datamode)
      log.debug("DEBUG: Login Mode: " + loginmode)

      # check to see that we have the required fields
      if username and password and where and description:
         log.debug("DEBUG: All required FIELDS found")
      else:
         log.error("ERROR: NOT all required fields found. Can't continue.")
         return 3

      # if statement that checks for Crypt:SSLeay -- no equivalent in python

   if noop != 0:
      log.warning("NOOP selected. No files will be transferred.")
      return 99

   # start the transfer timer
   starttime = time.time()

   ##### Begin file processing, Starts with finding all files in any directories passed to the script.
   if squishyuser:
      log.info("INFO: Processing Files")
   listoffiles = []  # List of files we're going to be transferring

   ##### Check to see if the item is a directory, or if it's a file.
   usefullpath = 0
   for arg in args:
      if os.path.isdir(arg):
         if re.search('\.\.\/|\~', arg):
            usefullpath = 1
         if debug:
            log.info("INFO: Processing Directory " + arg + " with " + description)
         for root, dirs, files in os.walk(arg):
            for name in files:
               if root:
                  listoffiles.append( root + "/" + name)
               else:
                  listoffiles.append( name)
      elif os.path.isfile(arg):
         listoffiles.append(arg)
         (dirname, filename) = os.path.split(arg)
         if re.search('\.\.|\~', arg):  ## for consistency with perl
            usefullpath = 1
      else:
         log.error("ERROR: file " + arg + " is an unrecognized filetype or non-existent")

   for i in listoffiles:
      log.debug("DEBUG: listoffiles element: " + i)
       

   # 107374182400 = 100GB in bytes
   sizeoftransfer = 0

   for i in listoffiles:
      filesize = os.path.getsize(i)
      if filesize > 107374182400:
         log.error("ERROR: Maximum Single File Size is 107374182400 bytes (100GB), file " + i + " is " + str(filesize) + " bytes.")
         return 4
      sizeoftransfer = sizeoftransfer + filesize

   if sizeoftransfer > 214748364800:
      log.error("ERROR: Maximum Transfer Size is 214748364800 bytes (200GB). This transfer is " + str(sizeoftransfer) + " bytes.")
      return 4

   log.debug("DEBUG: sizeoftransfer = " + str(sizeoftransfer))

   if len(listoffiles) <= 0:
      log.error("ERROR: No files found to transfer")
      return 4
   elif '(NOZIP)' in description:
      if squishyuser and not headless:
         if pyver == 3:
            confirm_nozip = input("You have requested the NOZIP option. USE WITH CAUTION!! Type YES to confirm: ")
         else:
            confirm_nozip = raw_input("You have requested the NOZIP option. USE WITH CAUTION!! Type YES to confirm: ")
         if confirm_nozip == 'YES':
            log.warn("WARNING: Directory Processing complete: Found " + str(len(listoffiles)) + " files AND we are not zipping since it was requested.")
         else:
            log.error("ERROR: You didn't type YES. Exiting....")
            return
      else:
         log.info("INFO: You have requested the NOZIP option. USE WITH CAUTION!!!!!!")
   elif len(listoffiles) > 1000:
      if squishyuser:
         log.info("INFO: Zipping files because they exceed 1000")
      # check to see how many files we have. If the number is greater than 1000, zip the files before throwing.

      zippedfiles = 1

      if verbose or debug:
         log.info("INFO: Zipping files because there are more than 200 files to transfer")

      if windows:
         zip = '7zip'
      else:
         zip = 'zip -q -@'

      zipfilename = transfer_timestamp + '.zip'
      
      filelist = open('/tmp/' + transfer_timestamp + '.files', 'w')
      while len(listoffiles) > 0:
         file = listoffiles.pop()
         filelist.write(file + '\n')
      filelist.close()

      log.debug("DEBUG: Zipping with command line: " + zip + " " + zipfilename + " < /tmp/" + transfer_timestamp + ".files")

      if windows:
         #### FIXME Need a zip line for Windows
         pass
      else:
         os.system(zip + " " + zipfilename + " < /tmp/" + transfer_timestamp + ".files")

      if not debug:
         os.unlink("/tmp/" + transfer_timestamp + ".files")

      if os.path.isfile(zipfilename):
         listoffiles.append(zipfilename)
         usefullpath = 0 #### we override this since everything is now in one file
      if len(listoffiles) <= 0:
         log.error("ERROR: Zip failure, please check status of ZIP")
         return 4
   else:
      log.debug("DEBUG: Directory Processing complete: Found " + str(len(listoffiles)) + " files.")

   if sequence > 0:
      sequence_local = "/var/tmp/swift." + sequence + ".sequence"
      sequence_id = 0
      
      ## Sequencing has been requested, therefore we need to create / open the transfer file
      if os.path.isfile(sequence_local):
         file = open(sequence_local, 'r')
         sequence_id = int(file.readline())
         file.close()
      else:
         sequence_id = 0

      file = open(sequence_local, 'w')
      file.write(str(sequence_id + 1))
      file.close()

      transferseq = open(transfer_timestamp + ".sequence", 'w')
      transferseq.write(str(sequence) + ':' + str(sequence_id) + '\n')
      transferseq.close()

      listoffiles.append(transfer_timestamp + ".sequence")

   ##### Send the server the information needed to continue transfer. Transfer can start after success.
   transfer_info_success = 0
   transfer_info_ctr = 0

   if squishyuser:
      log.info("INFO: Transmitting transfer information to server to start transfer")

   while not transfer_info_success:

      values = [('description', description),
                ('client', 'script'),
                ('Button', 'Set Transfer Info') ]
      for dest in postwhere:
         values.append( ('where_fields[]', dest))

      if pyver == 3:
         reqdata = urllib.parse.urlencode(values).encode("utf-8")
         req = urllib.request.Request(loginurl, reqdata) 
         try:
            resp = urllib.request.urlopen(req)
         except Exception, e: 
            if transfer_info_ctr > 3:
               log.error("ERROR: " + e.code + " : " + e.msg + "\nContact Support")
               return 9
            transfer_info_ctr = transfer_info_ctr + 1
            transfer_info_success = 0
            sleep(transfer_info_ctr**2)
         else:
            log.debug("DEBUG: Post Wherefield Submission Result: " + resp.read().decode("utf-8"))
            transfer_info_success = 1
      else:
         reqdata = urllib.urlencode(values)
         req = urllib2.Request(loginurl, reqdata) 
         try:
            resp = urllib2.urlopen(req)
         except Exception, e: 
            if transfer_info_ctr > 3:
               log.error("ERROR: " + e.code + " : " + e.msg + "\nContact Support")
               return 9
            transfer_info_ctr = transfer_info_ctr + 1
            transfer_info_success = 0
            sleep(transfer_info_ctr**2)
         else:
            log.debug("DEBUG: Post Wherefield Submission Result: " + resp.read())
            transfer_info_success = 1

   error_string = error_string + "Beginning File Upload.\n"

   if squishyuser:
      log.info("INFO: Uploading Files")

   for i in listoffiles:
      log.debug("DEBUG: listoffiles element: " + i)

   # now process the files, and upload each one to the server
   file_hash = {}
   for file in listoffiles:
      fail_part_ctr = 0
      fail_file = 0
      if not os.path.isdir(file):
         if os.path.isfile(file) and file not in file_hash:
            error_string = error_string + "F: " + file + "  "
            if verbose:
               log.info("INFO: Processing filename: " + file)
            dirpath = ""
            filename = ""

            if windows:
               file.replace('\\','/')

            (dirpath, filename) = os.path.split(file)
            log.debug("DEBUG: os.path.split: " + dirpath + " : " + filename)

            ## directory path normalization
            if usefullpath:
               dirpath = os.path.dirname(os.path.abspath(file))

            log.debug("DEBUG: Dirpath " + dirpath)
            log.debug("DEBUG: Filename " + filename) 
 

            filesize = os.path.getsize(file)
            transfersize = transfersize + filesize
            log.debug("DEBUG: filesize = " + str(filesize))

            numpartitions = int((filesize / partitionsize) + 1) ##### number of times the file has to be partitioned to be sent
            log.debug("DEBUG: number of Partititons = " + str(numpartitions))

            randomid = random.randint(0,int(time.time()))

            ##### Calculating the MD5 sum of each file, since the server expects us to send it (disabled 03-04-2011 by Durango, not necessary can be removed after testing).
            #### Warning -- this if clause has not been ported for universality between Python 2 and 3.
            if False:
               if verbose:
                  log.info("INFO: Calculating MD5 sum of " + file + ". This may take some time for large files")
               if pyver == 3:
                  m = hashlib.md5()
               else:
                  m = md5.new()
               filecontents = open(file, 'rb')
               while True:
                  data = filecontents.read(1048576) ### read 1 MB at a time
                  if not data:
                     break
                  m.update(data)
               filemd5 = m.hexdigest().upper()
            #### Calculating the MD5 sum of this file, 1MB at a time

            ##### now sending the file.
            if usefullpath:
               if dirpath[0] == '/':
                  dirpath = dirpath[1:]
               if verbose:
                  log.info("INFO: Sending relativePath = " + dirpath) 
               path = dirpath
            else:
               if dirpath:
                  if dirpath[0] == '/':
                     dirpath = dirpath[1:]
                  if verbose:
                     log.info("INFO: Sending relativePath = " + dirpath + '/' + filename)
                  path = dirpath + '/' + filename
               else:
                  if verbose:
                     log.info("INFO: Sending relativePath = .")
                  path = filename
           
            # strip leading directories if specified on the command line
            if cutdir:
               pathtoproc = os.path.normpath(path)
               patharray = pathtoproc.split(os.sep)
               if cutdir <= len(patharray):
                  if patharray[0] == '':
                     path = os.path.join( *tuple( patharray[cutdir+1:] ) )
                  else: 
                     path = os.path.join( *tuple( patharray[cutdir:] ) )
               else:
                  path = filename 

            # Make file chunks
            filehandle = open(file, 'rb')
            if pyver == 3:
               md5calc = hashlib.md5()
            else:
               md5calc = md5.new()

            ##### Send each part of the file.
            part = 0
            while part < numpartitions:
               part = part + 1
               fail_part_flag = 0
               size = partitionsize
               if part == numpartitions:
                  size = filesize % partitionsize
               ##### Read in up to partitionsize chunks of data and then write them back out to the server as post data
               log.debug("DEBUG: Partition size: " + str(size))
               buffer = filehandle.read(size)
               md5calc.update(buffer)
               while fail_part_flag == 0:
                  if part == numpartitions:
                     filemd5sum = md5calc.hexdigest().upper()
                     log.debug("DEBUG: Chunk MD5 = " + filemd5sum)
                     log.debug("DEBUG: relativepath = " + path) 
                     log.debug("DEBUG: name = " + filename)
                     log.debug("DEBUG: file = " + file)
                     values = [('fileId', str(randomid)),
                               ('partitionIndex', str(part-1)),
                               ('partitionCount', str(numpartitions)),
                               ('fileLength', str(filesize)),
                               ('relativePath', path),
                               ('name', filename),
                               ('md5', filemd5sum)]
                     fileval = [('file',file, buffer)]
                  else:
                     values = [('fileId', str(randomid)),
                               ('partitionIndex', str(part-1)),
                               ('partitionCount', str(numpartitions)),
                               ('fileLength', str(filesize)),
                               ('relativePath', path),
                               ('name', filename)]
                     fileval = [('file',file, buffer)]
                  req = create_post_multipart_req(partitionurl, values, fileval)
                  # if the file size is bigger than 2G, increase th timeout to an hour
                  if filesize > 2147483648:
                     default_timeout = 3600
                  else:
                     default_timeout = 180 
                  socket.setdefaulttimeout(default_timeout)

                  if pyver == 3:
                     try:
                        resp = urllib.request.urlopen(req)
                     except Exception, e: 
                        if transfer_info_ctr > 3:
                           log.error("ERROR: " + e.code + " : " + e.msg + "\nContact Support")
                           return 9
                     else:
                        respdata = resp.read()
                        log.debug("DEBUG: server response: " + respdata.decode("utf-8"))
                        if "Can't move uploaded file" in respdata.decode("utf-8"):
                           ### Retry upload of this piece
                           fail_part_ctr = fail_part_ctr + 1
                           fail_part_flag = 0
                        elif "Upload validation error" in respdata.decode("utf-8"):
                           ### Retry upload of this file
                           fail_file = fail_file + 1
                        elif "Missing XML File" in respdata.decode("utf-8") or "Transfertimestamp not set. Can't upload files(s)" in respdata.decode("utf-8"):
                           ### All other errors are Fatal
                           log.error("ERROR: Fatal Return message from partition code: " + respdata.decode("utf-8"))
                           return 9
                        else:
                           fail_part_flag = 1
   
                        log.debug("DEBUG: Post Wherefield Submission Result: " + resp.read().decode("utf-8"))
                  else:
                     try:
                        resp = urllib2.urlopen(req)
                     except Exception, e: 
                        if transfer_info_ctr > 3:
                           log.error("ERROR: " + e.code + " : " + e.msg + "\nContact Support")
                           return 9
                     else:
                        respdata = resp.read()
                        log.debug("DEBUG: server response: " + respdata)
                        if "Can't move uploaded file" in respdata:
                           ### Retry upload of this piece
                           fail_part_ctr = fail_part_ctr + 1
                           fail_part_flag = 0
                        elif "Upload validation error" in respdata:
                           ### Retry upload of this file
                           fail_file = fail_file + 1
                        elif "Missing XML File" in respdata or "Transfertimestamp not set. Can't upload files(s)" in respdata:
                           ### All other errors are Fatal
                           log.error("ERROR: Fatal Return message from partition code: " + respdata)
                           return 9
                        else:
                           fail_part_flag = 1
   
                        log.debug("DEBUG: Post Wherefield Submission Result: " + resp.read())

                  # Reset timeout to the default
                  default_timeout = 180 
                  if fail_part_ctr > 3:
                     log.error("ERROR: Failed to upload partition " + str(part) + " three times, Contact Support")
                     return 9
            log.debug("DEBUG: Chunk MD5 = " + md5calc.hexdigest().upper())

            filehandle.close()
            if fail_file and fail_file < 3:
               # file failed to upload
               listoffiles.append(file)
               log.warn("WARNING: File " + file + " failed to upload, retrying")
            elif fail_file >= 3:
               log.error("ERROR: File " + file + " failed to upload 3 times, no longer retrying")
            else:
               file_hash[file] = 1

   ##### Now that all of the pieces have been uploaded, click finish
   finalize_success = 0
   finalize_ctr = 0

   if squishyuser:
      log.info("INFO: Finalizing Transfer")

   while not finalize_success:
      values = [('client', 'script'),
                ('Button', 'Finalize Transfer')]
      if pyver == 3:
         reqdata = urllib.parse.urlencode(values).encode("utf-8")
         req = urllib.request.Request(loginurl, reqdata)
         try:
            resp = urllib.request.urlopen(req)
         except Exception, e:
            if finalize_ctr > 3:
               log.error("ERROR: " + e.code + " : " + e.msg + "\nContact Support")
               log.error("ERROR STRING:" + error_string)
               return 9
            else:
               finalize_ctr = finalize_ctr + 1
               finalize_success = 0
               sleep(finalize_ctr**2)
         else:
            log.debug("DEBUG: Finalize Transfer Submission Result: " + resp.read().decode("utf-8"))
            finalize_success = 1
      else:
         reqdata = urllib.urlencode(values)
         req = urllib2.Request(loginurl, reqdata)
         try:
            resp = urllib2.urlopen(req)
         except Exception, e:
            if finalize_ctr > 3:
               log.error("ERROR: " + e.code + " : " + e.msg + "\nContact Support")
               log.error("ERROR STRING:" + error_string)
               return 9
            else:
               finalize_ctr = finalize_ctr + 1
               finalize_success = 0
               sleep(finalize_ctr**2)
         else:
            log.debug("DEBUG: Finalize Transfer Submission Result: " + resp.read())
            finalize_success = 1


   ##### Now the server will start sending us back status information when we request it.
   loopstatus = 0

   if squishyuser:
      log.info("INFO: Waiting for Status")

   ##### Let the loop only run for max_status_pool_sec seconds (Default is an hour)

   status_pool_loop_num = int(max_status_poll_sec / status_poll_sec)

   for i in range(0,status_pool_loop_num):
      values = [('client', 'script'),
                ('status_username', username)]
      for dest in postwhere:
         values.append( ('status_where_fields[]', dest))

      if pyver == 3:
         reqdata = urllib.parse.urlencode(values).encode("utf-8")
         req = urllib.request.Request(loginurl + "?func=status", reqdata)
      else:
         reqdata = urllib.urlencode(values)
         req = urllib2.Request(loginurl + "?func=status", reqdata)

      if pyver == 3:
         try:        
            resp = urllib.request.urlopen(req)
         except Exception, e:
            log.error("ERROR: " + resp.read().decode("utf-8") + "\nContact Support")
            return 9
         else:
            statusmessage = resp.read().decode("utf-8")
            statuses = statusmessage.split('\n')
            log.debug("DEBUG: statusmessage = \n" + statusmessage)
            numstatus = len(statuses)
            status_message = ''
   
            log.debug("DEBUG: Number of status messages received: " + str(numstatus))
            #####FIXME: this probably should be created elsewhere, thi sis a crude guess at number of transfers
            numtransfers = numstatus
   
            ##### For each vlaue passed back from the server, process the state it came from.
            for status in statuses:
               if status == '':
                  continue
               if 'WAIT' in status:
                  regexsearch = re.search('WAIT:(.*):(.*)$', status)
                  if verbose:
                     log.info("INFO: " + regexsearch.group(2))
               elif 'DELIVERED' in status: 
                  regexsearch = re.search('DELIVERED:(.*):(.*)$', status)
                  if verbose:
                     log.info("INFO: " + regexsearch.group(2))
               elif 'RECEIVED' in status: 
                  regexsearch = re.search('RECEIVED:(.*):(.*)$', status)
                  if verbose:
                     log.info("INFO: " + regexsearch.group(2))
               elif 'FAILED' in status: 
                  regexsearch = re.search('FAILED:(.*):(.*)$', status)
                  if verbose:
                     log.info("INFO: " + regexsearch.group(2))
               elif 'STATUS' in status: 
                  regexsearch = re.search('STATUS:(.*)$', status)
                  loopstatus = regexsearch.group(1)
                  log.debug("DEBUG: status: " + status + " loopstatus: " + loopstatus)
                  error_string = error_string + str(i)
               else:
                  error_string = error_string + "\nUnknown status received. Contact Support. Status = " + status + "\n"
                  log.error("ERROR STRING: " + error_string)
                  return 9
               status_message = status
            if loopstatus == '0':
               log.info("INFO: Success transfer_timestamp = " + transfer_timestamp + " with size " + '{0:f}'.format(float(transfersize)/1000000000) + "G in " + str(float(time.time() - starttime)) + " sec")
               if zippedfiles:
                  if not debug:
                     os.unlink(zipfilename)
               return 0
            elif loopstatus == '2':
               error_string = error_string + "\nA transfer has failed. STATUS = " + status_message + "\n"
               log.error("ERROR STRING: " + error_string)
               return 1
               log.error("ERROR: " + resp.read().decode("utf-8") + "\nContact Support")
               return 9
         sleep(status_poll_sec)
      else:          
         try:        
            resp = urllib2.urlopen(req)
         except Exception, e:
            log.error("ERROR: " + resp.read() + "\nContact Support")
            return 9
         else:
            statusmessage = resp.read()
            statuses = statusmessage.split('\n')
            log.debug("DEBUG: statusmessage = \n" + statusmessage)
            numstatus = len(statuses)
            status_message = ''
   
            log.debug("DEBUG: Number of status messages received: " + str(numstatus))
            #####FIXME: this probably should be created elsewhere, thi sis a crude guess at number of transfers
            numtransfers = numstatus
   
            ##### For each vlaue passed back from the server, process the state it came from.
            for status in statuses:
               if status == '':
                  continue
               if 'WAIT' in status:
                  regexsearch = re.search('WAIT:(.*):(.*)$', status)
                  if verbose:
                     log.info("INFO: " + regexsearch.group(2))
               elif 'DELIVERED' in status: 
                  regexsearch = re.search('DELIVERED:(.*):(.*)$', status)
                  if verbose:
                     log.info("INFO: " + regexsearch.group(2))
               elif 'RECEIVED' in status: 
                  regexsearch = re.search('RECEIVED:(.*):(.*)$', status)
                  if verbose:
                     log.info("INFO: " + regexsearch.group(2))
               elif 'FAILED' in status: 
                  regexsearch = re.search('FAILED:(.*):(.*)$', status)
                  if verbose:
                     log.info("INFO: " + regexsearch.group(2))
               elif 'STATUS' in status: 
                  regexsearch = re.search('STATUS:(.*)$', status)
                  loopstatus = regexsearch.group(1)
                  log.debug("DEBUG: status: " + status + " loopstatus: " + loopstatus)
                  error_string = error_string + str(i)
               else:
                  error_string = error_string + "\nUnknown status received. Contact Support. Status = " + status + "\n"
                  log.error("ERROR STRING: " + error_string)
                  return 9
               status_message = status
            if loopstatus == '0':
               log.info("INFO: Success transfer_timestamp = " + transfer_timestamp + " with size " + str(float(transfersize)/1000000000) + "G in " + str(float(time.time() - starttime)) + " sec") 
               if zippedfiles:
                  if not debug:
                     os.unlink(zipfilename)
               return 0
            elif loopstatus == '2':
               error_string = error_string + "\nA transfer has failed. STATUS = " + status_message + "\n"
               log.error("ERROR STRING: " + error_string)
               return 1
               log.error("ERROR: " + resp.read() + "\nContact Support")
               return 9
         sleep(status_poll_sec)
   ##### if we exit loop, we must have had a timeout of our loop.
   if verbose:
      log.error("ERROR: Transfer status timeout")

   return 2

# end main()

def os_detection():
   global OS

   OS = platform.platform()
   if 'Linux' in OS:
      OS = 'UNIX'
   elif 'Darwin' in OS:
      OS = 'MACINTOSH'
   elif 'Wind' in OS:
      OS = 'WINDOWS'
   else:
      OS = 'UNIX'
# end os_detection()

def new_feeder_address():

   global last_index
   global feeder_index
   global feeder_counter
   global error_string
   global feederarray
   global debug

   feeder_counter = feeder_counter + 1
   if feeder_counter > len(feederarray)*4 :
      error_string = error_string + 'ERROR: Too many attempts to use different feeders. Contact support'
   # if there is only one entry we can't be picky
   if len(feederarray) == 1:
      feeder_index = 0
   else:
      # Pick a random from the array as long as its not the last one we had
      while last_index == -1 or last_index == feeder_index: 
         feeder_index = random.randint(0,len(feederarray)-1)
         if last_index == -1:
            last_index = feeder_index

   last_index = feeder_index
   feeder_ip = feederarray[feeder_index]
   log.debug("DEBUG: Setting Feeder IP Address to " + feeder_ip)
   return feeder_ip

# end new_feeder_address()

def create_post_multipart_req(url, fields, files):
   content_type, body = encode_multipart_formdata(fields, files)
   headers = {'Content-Type': content_type,
              'Content-Length': str(len(body))}
   if pyver == 3:
      req = urllib.request.Request(url, body, headers)
   else:
      req = urllib2.Request(url, body, headers)
   return req
# end post_multipart()

def encode_multipart_formdata(fields, files):
   if pyver == 3:
      BOUNDARY = email.generator._make_boundary()
      CRLF = '\r\n'
      L = []
      for (key, value) in fields:
         L.append(bytes('--' + BOUNDARY,"utf-8"))
         L.append(bytes('Content-Disposition: form-data; name="' + key + '"',"utf-8"))
         L.append(bytes('',"utf-8"))
         L.append(bytes(value,"utf-8"))
      for (key, filename, value) in files:
         L.append(bytes('--' + BOUNDARY,"utf-8"))
         L.append(bytes('Content-Disposition: form-data; name="' + key + '"; filename="' + filename + '"',"utf-8"))
         L.append(bytes('Content-Type: multipart/form-data',"utf-8"))
         L.append(bytes('',"utf-8"))
         L.append(value)
      L.append(bytes('--' + BOUNDARY + '--',"utf-8"))
      L.append(bytes('',"utf-8"))
      body = bytes(CRLF,"utf-8").join(L)
      content_type = 'multipart/form-data; boundary=%s' % BOUNDARY
   else:
      BOUNDARY = mimetools.choose_boundary()
      CRLF = '\r\n'
      L = []
      for (key, value) in fields:
         L.append('--' + BOUNDARY)
         L.append('Content-Disposition: form-data; name="%s"' % key)
         L.append('')
         L.append(value)
      for (key, filename, value) in files:
         L.append('--' + BOUNDARY)
         L.append('Content-Disposition: form-data; name="%s"; filename="%s"' % (key, filename))
         L.append('Content-Type: %s' % 'multipart/form-data' )
         L.append('')
         L.append(value)
      L.append('--' + BOUNDARY + '--')
      L.append('')
      body = CRLF.join(L)
      content_type = 'multipart/form-data; boundary=%s' % BOUNDARY

   return content_type, body
# end encode_multipart_formdata()



####### Return Code Documentation ###
##### 
##### This code will return a specific code for various exit conditions. 
#####
##### 0 = Successful transfer
##### 1 = A transfer has failed
##### 2 = Status Timeout 
##### 3 = Configuration Error
##### 4 = Invalid File List
##### 5 = Invalid Login Information
##### 9 = Server Error
##### 10 = Could not run headless through python module call
##### 99 = Successful noop run
#####
#####################################


if __name__ == "__main__":
   sys.exit(main())
