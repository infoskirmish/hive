SHELL=/bin/bash

# To assist in cross-compiling
CC=gcc
AR=ar
RANLIB=ranlib
LDFLAGS=

##########################################################################
# Defaults
CFLAGS  = -Wall -W -Wdeclaration-after-statement

.PHONY: ARCH_BUILD
include ARCH_BUILD

$(info )
$(info =========================================================================)
ifdef TARGET
  ifdef ARCH_BUILD_STRINGS
    ifneq ("$(ARCH_BUILD_STRINGS)", "$(TARGET)")
      $(info .  Rebuilding STRINGS for $(TARGET))
      STRINGS_BUILD_OPTS:=clean
    else
      STRINGS_BUILD_OPTS:=
      $(info .  STRINGS for $(TARGET) previously built.)
    endif
  else
    STRINGS_BUILD_OPTS:=clean
    $(info .  Building STRINGS for $(TARGET))
  endif
  
  ifeq ("$(TARGET)", "linux-x86")
    include ../Makefile-include.linux-x86
  else ifeq ("$(TARGET)", "linux-x86_64")
    include ../Makefile-include.linux-x86_64
  else ifeq ("$(TARGET)", "mikrotik-mips")
    include ../Makefile-include.mikrotik-mips
  else ifeq ("$(TARGET)", "mikrotik-mipsel")
    include ../Makefile-include.mikrotik-mipsel
  else ifeq ("$(TARGET)", "mikrotik-x86")
    include ../Makefile-include.mikrotik-x86
  else ifeq ("$(TARGET)", "mikrotik-ppc")
    include ../Makefile-include.mikrotik-ppc
  else ifeq ("$(TARGET)", "ubiquiti-mips")
    include ../Makefile-include.ubiquiti-mips
  else ifeq ("$(TARGET)", "avtech-arm")
    include ../Makefile-include.avtech-arm
  else ifeq ("$(TARGET)", "arm")
    include ../Makefile-include.arm
  else ifeq ("$(TARGET)", "solaris-sparc")
    include ../Makefile-include.solaris-sparc
  else ifeq ("$(TARGET)", "solaris-x86")
    include ../Makefile-include.solaris-x86
  else
    TARGET :=
  endif

  ifdef DEBUG
    CFLAGS += -g -DDEBUG -O0
  else
    CFLAGS += -Os
  endif
  $(info =========================================================================)
  $(info TARGET: $(TARGET))
  $(info SRCDIR: $(PWD))
  $(info .   CC: $(CC))
  $(info CFLAGS: $(CFLAGS))
  $(info =========================================================================)
  $(info )
endif

##########################################################################

OBJS= string_utils.o

.SILENT:

string_utils.o: string_utils.c $(STRINGS_BUILD_OPTS)
	printf "\tCC\t\t$<\n"
	$(CC) $(CFLAGS) -c $<
	echo "ARCH_BUILD_STRINGS=$(TARGET)" > ARCH_BUILD

clean: 
	printf "\tCLEANING:\t$(CURDIR)\n"
	rm -f *.o 
	>ARCH_BUILD

.c.o:
	printf "\tCC\t\t$<\n"
	$(CC) $(CFLAGS) -c $<
