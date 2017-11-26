#! /usr/bin/python

import ConfigParser
import getopt
import logging
import logging.handlers
import os
import sys
import time

USAGE = """
usage: ./cleanUp.py [-cdh]

  Cleans a set of directories by recursively deleting every file in the
  directory that is older than a specified maximum age based on last
  modification time.  Subdirectories are deleted if they are both empty and
  older than the maximum age.

  The directories to clean and their corresponding maximum ages are set in a 
  configuration file.  

  All flags are optional.

  -c, --config FILE
         Use FILE as the configuration file.  If not specified, FILE defaults to
         'cleanUp.cfg'. 

  -d, --debug
         Print debug messages.

  -h, --help
         Print this message.

Config File
  The script requires a config file with the following format.

   [config]
   baseDir: <directory>                  # All relative directories will be
                                         # relative to this value.  Defaults to
                                         # the current directory.

   ageUnit: <seconds|minutes|hours|days> # The units to be associated with the
                                         # ages in the [cleanUpDirectories]
                                         # section.  Defaults to 'days'.

   [cleanUpDirectories]                  # list of directories to clean along
   <directory>: <age>                    # with corresponding maximum ages.
   <directory>: <age> 

   Example:
            [config]
            baseDir: /home/data
            ageUnit: hours

            [cleanUpDirectories]
            relativeDir:  12
            /absoluteDir: 1
"""


# Globals
#     NOW
#         The current time in seconds.  Used to determine if a file or directory
#         is old enough to delete.
#
#     LOGGER
#         The Logger object used for all script logging.
#   
#     LOG_FILE
#         Name of log file
#
#     DEFAULT_CONFIG_FILE
#         The config file to use as a default if none is specified.
#
#     CLEAN_DIR_SECTION
#         The named of config file section containing the list of directories to
#         clean.
#
#     CONFIG_SECTION
#         The named of configuration section of the config file.
#
#     BASE_DIR_OPTION
#         The name of the base directory parameter in the config file.
#
#     AGE_UNIT_OPTION
#         The name of the age unit parameter in the config file.
#
#     DEFAULT_AGE_UNITS
#         The default age unit to use if none is specified.
#
#     AGE_UNIT_MULT
#         A dictionary where each key is an age unit and the values are
#         multipliers to get the equivelent number of seconds.
NOW                 = time.time()
LOGGER              = logging.getLogger()
LOG_FILE            = "clean.log"
DEFAULT_CONFIG_FILE = "cleanUp.cfg"
CLEAN_DIR_SECTION   = "cleanUpDirectories"
CONFIG_SECTION      = "config"
BASE_DIR_OPTION     = "baseDir"
AGE_UNIT_OPTION     = "ageUnit"
DEFAULT_AGE_UNITS   = "DAYS"
AGE_UNIT_MULT       = {'SECOND':1,  'SECONDS':1,
                       'MINUTE':60, 'MINUTES':60,
                         'HOUR':3600, 'HOURS':3600,
                          'DAY':86400, 'DAYS':86400 }



# Simple Exception class for our own exceptions
class CleanError(Exception):
   def __init__(self, value):
      self.value = value
   def __str__(self):
      return repr(self.value)



# removeIfOldAndEmpty(path, ageLimit, age) -> bool
#     path     - Path to the file or directory to (possibly) remove.
#     ageLimit - Age limit in seconds.  Anything that has been modified in the
#                last ageLimit seconds will not be deleted.  
#     age      - number of seconds since path was last modified.
#
# Deletes path if it has not been modified in the last ageLimit seconds.
# (if age > ageLimit)  If path is a directory then the directory must also be
# empty in order to be deleted.  Returns True if path was deleted, false
# otherwise.  
def removeIfOldAndEmpty(path, ageLimit, age):
   didRemove = False

   if age > ageLimit:
      LOGGER.debug("Deleting %s, age %i seconds." % (path, age))

      try:
         if os.path.isdir(path):
            if len(os.listdir(path)) == 0:
               os.rmdir(path)
               didRemove = True
         else:
            os.unlink(path)
            didRemove = True
      except OSError, e:
         LOGGER.error("Could not delete %s. %s" % (path, e))

   return didRemove



# clean(dir, ageLimit) -> int
#     dir      - The directory to clean.   
#     ageLimit - Age limit in seconds.  Anything that has been modified in the
#                last ageLimit seconds will not be deleted.  
# 
# Cleans the directory dir by recursively deleteing any fiies that have not
# been modified in the last ageLimit seconds.  Sub-directories are deleted if
# they are empty and have not been deleted in the last ageLimt seconds.
# Returns and int count of the total number of files and directories that were
# deleted. 
def clean(dir, ageLimit):
   # This is the recursive count of the number of files and directories
   # deleted.
   count = 0

   try:
      for name in os.listdir(dir):
         path = os.path.join(dir, name)

         # We calculate age here (instead of in the removeIfOldAndEmpty method)
         # because for directories we want to use the last modification time
         # before we started deleting files.
         age = NOW - os.path.getmtime(path)

         if os.path.isdir(path):
            LOGGER.debug("Cleaning %s." % path)
            count += clean(path, ageLimit)

         if removeIfOldAndEmpty(path, ageLimit, age):
            count += 1
   except OSError, e:
      LOGGER.error("Could not clean %s. %s" % (dir, e))

   return count



try:
   # Start with the config file set to the default.  We will change this if the
   # -c of --config fag is set.
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
   for o, a in getopt.getopt(sys.argv[1:], "c:ddh", ["config=", "debug", "help"])[0]:
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

   LOGGER.info(" Starting cleanUp.py")

   # Verify that config file exists.
   LOGGER.debug("Config file:     %s" % configFile)

   if not os.path.isfile(configFile):
      raise CleanError("Config file %s does not exist." % configFile)

   # Read config file
   config = ConfigParser.RawConfigParser()
   config.optionxform = str # case sensitive
   config.read(configFile)

   # Read the baseDir from the config file.  If it is not specified default to
   # an empty string which will cause the current directory to be the baseDir.
   baseDir = ""
   if config.has_option(CONFIG_SECTION, BASE_DIR_OPTION):
      baseDir = config.get(CONFIG_SECTION, BASE_DIR_OPTION)

   LOGGER.debug("Base Directory:  %s" % baseDir)

   # Read the ageUnit from the config file.  If is is not specified use the
   # default.
   ageUnit = DEFAULT_AGE_UNITS
   if config.has_option(CONFIG_SECTION, AGE_UNIT_OPTION):
      ageUnit = config.get(CONFIG_SECTION, AGE_UNIT_OPTION)

   LOGGER.debug("Age Unit:        %s" % ageUnit)
   
   # Look up the multiplier for the ageUnit.
   ageMultiplier = 1
   if ageUnit.upper() in AGE_UNIT_MULT:
      ageMultiplier = int(AGE_UNIT_MULT[ageUnit.upper()])
   else:
      raise CleanError("Unknown ageUnit '%s' in config file %s." % (ageUnit, configFile))

   LOGGER.debug("Age Multiplier:  %i" % ageMultiplier)

   # Verify that the config file has a "cleanUpDirectories" section
   cleanDirs = []
   if config.has_section(CLEAN_DIR_SECTION):
      cleanDirs = config.items(CLEAN_DIR_SECTION)
   else:
      raise CleanError("Config file %s does not contain the '%s' section." % (configFile, CLEAN_DIR_SECTION))

   # Warn if there are no directories to clean
   if len(cleanDirs) == 0:
      LOGGER.warn("No directories to clean.")

   # Clean the specified directories for each (directory, age limit) tuple
   # listed in the config file.
   for (dir, ageLimit) in cleanDirs:
      path = os.path.join(baseDir, dir) 

      try:
         ageLimitInSeconds = int(ageLimit) * ageMultiplier
      except ValueError, e:
         LOGGER.error("Can not clean %s.  Invalid age limit: '%s',  Age limit must be an int." % (path, ageLimit))
         continue

      LOGGER.info(" Cleaning %s. Age limit: %s %s (%s seconds)" % (path, ageLimit, ageUnit, ageLimitInSeconds))
      count = clean(path, ageLimitInSeconds)
      LOGGER.info(" Removed %i files and directories from %s." % (count, path))

except getopt.GetoptError, e:
   # An invalid comman line option as used.
   LOGGER.critical(e)
   print USAGE

except (ConfigParser.ParsingError, ConfigParser.NoSectionError, CleanError), e:
   LOGGER.critical(e)

else:
   LOGGER.info(" Done")
