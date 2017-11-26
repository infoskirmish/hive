####################################################
# Architecture-agnostic Makefile for Hive crypto
####################################################
 CC := $(shell which gcc)
# Defaults
CFLAGS	:= -Wall -W -Wdeclaration-after-statement
COMMON = ..
SSL = $(COMMON)/ssl
INCLUDES := -I$(COMMON)/debug -I../ssl/include

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
else ifeq ("$(TARGET)", "arm")
  include ../Makefile-include.arm
else ifeq ("$(TARGET)", "solaris-sparc")
  include ../Makefile-include.solaris-sparc
else ifeq ("$(TARGET)", "solaris-x86")
  include ../Makefile-include.solaris-x86
else
  TARGET :=
endif

##########################################################################
AR = ar

LIBCRYPTO = libcrypto.a

LDFLAGS = -lpthread -lutil

CRYPTO_OBJS = crypto.o init_crypto_strings.o loki_utils.o dhExchange.o

PS = process_list.c
NETSTAT = netstat_an.c netstat_rn.c
IFC = ifconfig.c

CFLAGS += $(INCLUDES)

DBGFLAGS += -DDEBUG -D_DEBUG -g

STRIPFLAGS += --strip-all --remove-section=.comment --remove-section=.note

# Debug Options
ifdef DEBUG
  ifeq ("$(DEBUG)", "1")
    CFLAGS += -g -O0
  endif
  ifeq ("$(DEBUG)", "2")
    CFLAGS += -g -DDEBUG -O0
  endif
  ifeq ("$(DEBUG)", "3")
    CFLAGS += -g -DDEBUG -O0
  endif
else
  CFLAGS += -O2
endif

.SILENT:
.PHONY: $(CRYPTO_BUILD_OPTS)
$(info )
$(info =========================================================================)
ifdef TARGET
  ifdef ARCH_BUILD_CRYPTO
    ifneq ("$(ARCH_BUILD_CRYPTO)", "$(TARGET)")
      $(info .  Rebuilding libcrypto for $(TARGET))
      CRYPTO_BUILD_OPTS:=clean
    else
      CRYPTO_BUILD_OPTS:=
      $(info .  libcrypto for $(TARGET) previously built.)
    endif
  else
    CRYPTO_BUILD_OPTS:=clean
    $(info .  Building libcrypto for $(TARGET))
  endif
  
  $(info =========================================================================)
  $(info TARGET: $(TARGET))
  $(info SRCDIR: $(PWD))
  $(info .   CC: $(CC))
  $(info CFLAGS: $(CFLAGS))
  $(info =========================================================================)
  $(info )
endif

$(LIBCRYPTO): $(CRYPTO_BUILD_OPTS) crypto_proj_strings.h init_crypto_strings.o $(CRYPTO_OBJS)
	printf "\tAR\t$@\n"
	$(AR) $(ARFLAGS) $@ $(CRYPTO_OBJS)
	echo "ARCH_BUILD_CRYPTO=$(TARGET)" > ARCH_BUILD
	printf "=========================================================================\n"
	printf "\t$@ COMPLETE\n"
	printf "=========================================================================\n"

.PHONY: crypto_strings
crypto_proj_strings.h: init_crypto_strings.c
init_crypto_strings.o: init_crypto_strings.c 
init_crypto_strings.c: crypto_strings.txt 
	python mod_gen_cryptostring_header.py crypto_strings.txt CRYPTO_STRINGS

.PHONY: clean
clean:
	printf "\tCLEANING:\t$(CURDIR)\n"
	rm -f $(LIBCRYPTO) $(CRYPTO_OBJS)
	rm -f init_crypto_strings.*
	rm -f crypto_strings_main.h
	rm -f crypto_proj_strings.h
	>ARCH_BUILD

.PHONY: distclean
distclean: clean
	printf "\tDIST CLEANING:\t$(CURDIR)\n"
	rm -f $(LIBCRYPTO)

.c.o:
	printf "\tCC\t$<\n"
	$(CC) $(CFLAGS) -c $<
