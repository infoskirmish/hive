#! /usr/bin/python

import cPickle
import ConfigParser
import getopt
import getpass
import logging
import logging.handlers
import os
import re
import stat
import sys
import time
import swift_upload as swift

USAGE = """
Usage: ./hcOwt.py [-cdh]

  Reads a list of owt directories from a config file and sends all updated files
  in those directories across the OWT in a single owt tgz file.  "updated" files
  are files that have been added or changed since the last time the script ran.

  -c, --config FILE
         Use FILE as the configuration file.  If not specified, FILE defaults to
         'hcOwt.cfg'.

  -d, --debug
         Print debug messages.

  -h, --help
         Print this usage.


Config File:
  The script requires a config file with the following format.

   [config]
   name:<name>             # A name that will be included in the owt tgz file
                           # name.  The name will also be used as the top level
                           # directory in the tgz file.
   feeder:<feeder ip>      # OWT feeder ip.
   where:<where>           # OWT where field.
   username:<username>     # OWT username.
   password:<password>     # OWT password.

   [owtDirectories]        # List of directories to owt.  The "updated" files
   <directory>:<owt path>  # in each directory will be OWT'd.  Each item in
   <directory>:<owt path>  # the list has a <directory> and an <owt path>
                           #    <directory> - The absolute path to a a
                           #                  directory of files to OWT.
                           #    <owt path>  - The path in the tgz owt file to
                           #                  place updated files from this
                           #                  directory.

   Example:
            [config]
            name:     ham_eggs_logs
            feeder:   111.111.111.111
            where:    somewhere
            username: username
            password: password

            [owtDirectories]
            /var/log/ham: .
            /eggs:        eggs 

            The corresponding owt tgz file would look something like:

                 ham_eggs_logs
                 |-- ham file
                 |-- ham file
                 `-- eggs
                     |-- eggs file
                     `-- eggs file


Notes:
  - In order to determine which files have been updated, the script stores last
    modified times in the file ".hcOwt.data".  Deleting this file will cause the
    script to OWT all files in the owt directories. 

  - The script is not recursive.  Only updated files at the top level of the
    owt directories will be OWT'd.  Subdirectories will be ignored.

  - The script logs to a file called hcOwt.log.  The log file is rotated at 500k
    and 2 backups of old logs are retained.
"""

# Globals
#     WD
#         The script working directory.  This is the directory that the script
#         is executed from.  
#
#     NOW
#         The current time in seconds.  Used in the name of the tgz file and its
#         top directory.
#
#     LOGGER
#         The Logger object used for all script logging.
#   
#     LOG_FILE
#         Name of log file
#
#     TRANSFER_DIR
#         Name of the directory to use for building transfer tgz files.
#
#     PERSIST_FILE
#         Name of the file used to persist the last modification date of files.
#         Used to determine which files have been updated since the last run.
#
#     DEFAULT_CONFIG_FILE
#         The config file to use as a default if none is specified.
#
#     OWT_DIR_SECTION
#         The named of config file section containing the list of directories to
#         owt.
#
#     CONFIG_SECTION
#         The named of configuration section of the config file.
#
#     FEEDER_OPTION
#         The name of the feeder ip parameter in the config file.
#
#     WHERE_OPTION
#         The name of the where parameter in the config file.
#
#     USERNAME_OPTION
#         The name of the username parameter in the config file.
#
#     PASSWORD_OPTION
#         The name of the password parameter in the config file.
#
#     NAME_OPTION
#         The name of the name parameter in the config file.
#
#     CURL_EXE
#         Executable to run using curl with the OWT.
#
#     COOKIE_FILE
#         Cookie file that is used and deleted as part of the OWT.
WD                  = os.getcwd()
NOW                 = time.localtime()
LOGGER              = logging.getLogger()
LOG_FILE            = "hcOwt.log"
TRANSFER_DIR        = "hcOwt.transfer"
PERSIST_FILE        = ".hcOwt.data"

DEFAULT_CONFIG_FILE = "hcOwt.cfg"
OWT_DIR_SECTION     = "owtDirectories"
CONFIG_SECTION      = "config"
FEEDER_OPTION       = "feeder"
WHERE_OPTION        = "where"
USERNAME_OPTION     = "username"
PASSWORD_OPTION     = "password"
NAME_OPTION         = "name"


# Class for our own excpetions
class OwtError(Exception):
   def __init__(self, value):
      self.value = value
   def __str__(self):
      return repr(self.value)



# Run a command and return the response 
def run(command, logger=LOGGER):
   logger.debug("Command: %s" % command)
   response = os.popen(command, 'r').read()
   logger.debug("Response: %s" % response)

   return response

   

# Apply any flags from the command line.  
try:
   # Start with the config file set to the default.  We will change this if the
   # -c of --config flag is set.
   configFile = DEFAULT_CONFIG_FILE

   # Set up some simple info level console logging.  We will change this to debug
   # later if we see the -d or --debug flag.
   formatter = logging.Formatter("%(asctime)s -- %(levelname)s -- %(message)s")

   consoleHandler = logging.StreamHandler()
   consoleHandler.setLevel(logging.INFO)
   consoleHandler.setFormatter(formatter)

   fileHandler = logging.handlers.RotatingFileHandler(LOG_FILE, 
                                                      maxBytes=512000, # 500k
                                                      backupCount=2)
   fileHandler.setLevel(logging.INFO)
   fileHandler.setFormatter(formatter)

   LOGGER.setLevel(logging.INFO)
   LOGGER.addHandler(consoleHandler)
   LOGGER.addHandler(fileHandler)

   # Apply any flags from the command line.
   for o, a in getopt.getopt(sys.argv[1:], "c:dh", ["config=", "debug", "help"])[0]:
      if o in ("-d", "--debug"):
         consoleHandler.setLevel(logging.DEBUG)
         fileHandler.setLevel(logging.DEBUG)
         LOGGER.setLevel(logging.DEBUG)
         LOGGER.debug("Running in debug mode")
      elif o in ("-c", "--config"):
         configFile = a
      elif o in ("-h", "--help"):
         print USAGE
         sys.exit()

   LOGGER.info(" Starting OWT script...")

   # Verify that config file exists.
   LOGGER.debug("Config file:     %s" % configFile)

   if not os.path.isfile(configFile):
      raise OwtError("Config file %s does not exist." % configFile)

   # Read config file
   config = ConfigParser.RawConfigParser()
   config.optionxform = str # case sensitive
   config.read(configFile)

   feeder   = config.get(CONFIG_SECTION, FEEDER_OPTION)
   where    = config.get(CONFIG_SECTION, WHERE_OPTION)
   username = config.get(CONFIG_SECTION, USERNAME_OPTION)
   password = config.get(CONFIG_SECTION, PASSWORD_OPTION)
   name     = config.get(CONFIG_SECTION, NAME_OPTION)

   LOGGER.debug("feeder:   %s" % feeder)
   LOGGER.debug("where:    %s" % where)
   LOGGER.debug("username: %s" % username)
   LOGGER.debug("password: %s" % password)
   LOGGER.debug("name:     %s" % name)

   owtDirs = config.items(OWT_DIR_SECTION)
   if len(owtDirs) == 0:
      raise OwtError("No directories to owt.")

   # Use the current time to generate a filename for the upload file.  This is
   # the file that will be owt'ed.
   timeStr = "%04d%02d%02d%02d%02d%02d" % (NOW[0], NOW[1], NOW[2],
                                            NOW[3], NOW[4], NOW[5])
   transferDir = os.path.join(TRANSFER_DIR, timeStr)

   # Hash of mtimes to file paths from last run.  Populated from the
   # PERSIST_FILE
   old     = {}

   # Hash of current mtimes to file paths.
   new     = {}

   # load the persisted mtimes to compare against the current ones.
   LOGGER.debug("Loading persist file %s" % PERSIST_FILE)
   if os.path.isfile(PERSIST_FILE):
      input = open(PERSIST_FILE, 'rb')
      old = cPickle.load(input)
      input.close()

   LOGGER.info(" Getting updated files to owt.")

   # Read through each owt directory looking for files.  For each file, store 
   # the current mtime in the "new" hash.  Then compare the new mtime to the
   # mtime in the "old" hash.  Files that have changed will be copied to the 
   # TRANSFER_DIR to be included in the owt.  The TRANSFER_DIR is left as it
   # after the script runs and will need to be cleaned up by another process.
   for (dir, dest) in owtDirs:
      if not os.path.isdir(dir):
         LOGGER.warn("%s is not a directory" % dir)
         continue
         
      changed = []

      LOGGER.debug("Looking in %s" % dir)

      for file in os.listdir(dir):
         filePath = os.path.join(dir, file)
         if os.path.isfile(filePath):
            msg = "     %s - no change" % file  # we will change this msg if needed
            new[filePath] = os.stat(filePath)[stat.ST_MTIME]

            if (filePath not in old) or old[filePath] != new[filePath]:
               changed.append(filePath)
               msg = "     %s - CHANGED" % file 

            LOGGER.debug(msg)

      # Only created the required path in the TRANSFER_DIR if there are 
      # updated files to owt.  Note the "-p" flag in the "mkdir" command.  This
      # will create any missing parent directories as needed including the
      # TRANSFER_DIR itself.
      if len(changed) > 0: 
         destPath = os.path.join(transferDir, name, dest)
         os.system("mkdir -p %s" % destPath)

         for file in changed:
            os.system("cp %s %s" % (file, destPath))


   # Now we have gone through all of the owt dirs.  If the TRANSFER_DIR was 
   # not created then no updated files were found and we are done.  Otherwise
   # start the owt.
   if not os.path.exists(transferDir):
      LOGGER.info(" Nothing new to owt.")
   else:
      # Tar up the current direcotry into the upload file.  This script itself
      # will be excluded from the tar.   
      uploadFile = "%04d%02d%02d%02d%02d%02d_%s.tgz" % (NOW[0], NOW[1], NOW[2],
                                                     NOW[3], NOW[4], NOW[5], name)
      LOGGER.info(" Creating SWIFT file %s in %s ..." % (uploadFile, transferDir))

      os.chdir(transferDir)
      os.system("tar cfz %s *" % uploadFile)

      swift_retval = swift.upload( uploadFile, username, password, uploadFile, where, feeder )

      if( swift_retval == 0 ):
         LOGGER.info(" Success") 
      else:
         raise OwtError("SWIFT Error: " + str(swift_retval))

      # If the owt worked then persist the new mtimes in the "new" hash to the
      # PERSIST file.  They will be used as the old mtimes on the next run.
      LOGGER.debug("Saving persist file %s" % PERSIST_FILE)
      os.chdir(WD)
      output = open(PERSIST_FILE, 'wb')
      cPickle.dump(new, output, cPickle.HIGHEST_PROTOCOL)
      output.close()

except getopt.GetoptError, e:
   # An invalid comman line option as used.
   LOGGER.critical(e)
   print USAGE

except (ConfigParser.ParsingError, ConfigParser.NoSectionError, ConfigParser.NoOptionError, OwtError), e:
   LOGGER.critical(e)

except KeyboardInterrupt:
   print
   LOGGER.warn("Script canceled by user") 

