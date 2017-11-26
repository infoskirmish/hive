####################################################
# Architecture-agnostic Makefile for Debugging
####################################################
 CC := $(shell which gcc)
# Defaults
CFLAGS	= -Wall -W -Wdeclaration-after-statement
COMMON = ..
LIBDEBUG = $(COMMON)/debug
INCLUDES = -I.

.PHONY: ARCH_BUILD linux-x86
include ARCH_BUILD

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
else ifeq ("$(TARGET)", "solaris-sparc")
  include ../Makefile-include.solaris-sparc
else ifeq ("$(TARGET)", "solaris-x86")
  include ../Makefile-include.solaris-x86
else
  TARGET :=
endif

##########################################################################
AR = ar

LIBDEBUG = libdebug.a

OBJS = debug.o

CFLAGS += $(INCLUDES)
STRIPFLAGS += --strip-all --remove-section=.comment --remove-section=.note

# Debug Options
ifdef DEBUG
  CFLAGS += -g -DDEBUG -O0
else
  CFLAGS += -Os
endif

.SILENT:
.PHONY: $(DEBUG_BUILD_OPTS)
.PHONY: all

$(info )
$(info =========================================================================)
ifdef TARGET
  ifdef ARCH_BUILD_DEBUG
    ifneq ("$(ARCH_BUILD_DEBUG)", "$(TARGET)")
      $(info .  Rebuilding libdebug for $(TARGET))
      DEBUG_BUILD_OPTS:=clean
    else
      DEBUG_BUILD_OPTS:=
      $(info .  libdebug for $(TARGET) previously built.)
    endif
  else
    DEBUG_BUILD_OPTS:=clean
    $(info .  Building libdebug for $(TARGET))
  endif
  
  $(info =========================================================================)
  $(info TARGET: $(TARGET))
  $(info SRCDIR: $(PWD))
  $(info .   CC: $(CC))
  $(info CFLAGS: $(CFLAGS))
  $(info =========================================================================)
  $(info )
endif

$(LIBDEBUG): $(DEBUG_BUILD_OPTS) $(OBJS)
	printf "\tAR\t$@\n"
	$(AR) $(ARFLAGS) $@ $(OBJS)
	echo "ARCH_BUILD_DEBUG=$(TARGET)" > ARCH_BUILD
	printf "=========================================================================\n"
	printf "\t$@ COMPLETE\n"
	printf "=========================================================================\n"

.PHONY: clean
clean:
	printf "\tCLEANING:\t$(CURDIR)\n"
	rm -f $(LIBDEBUG) $(OBJS)
	>ARCH_BUILD

.PHONY: distclean
distclean: clean
	printf "\tDIST CLEANING:\t$(CURDIR)\n"
	rm -f $(LIBDEBUG)

.c.o:
	printf "\tCC\t$<\n"
	$(CC) $(CFLAGS) -c $<
