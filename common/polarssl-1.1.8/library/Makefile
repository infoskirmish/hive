# Makefile for PolarSSL
# Also see "include/polarssl/config.h"
LIBDIR = .
CC := $(shell which gcc)
ARFLAGS = rs

.PHONY: ARCH_BUILD
include ARCH_BUILD

$(info )
$(info =========================================================================)
ifdef TARGET
  ifdef ARCH_BUILD_SSL
    ifneq ("$(ARCH_BUILD_SSL)", "$(TARGET)")
      $(info .  Rebuilding SSL library for $(TARGET))
      SSL_BUILD_OPTS:=clean
    else
      SSL_BUILD_OPTS:=
      $(info .  SSL library for $(TARGET) previously built.)
    endif
  else
    SSL_BUILD_OPTS:=clean
    $(info .  Building SSL library for $(TARGET))
  endif
  
  ifeq ("$(TARGET)", "linux-x86-client")
    include ../../Makefile-include.linux-x86-client
  else ifeq ("$(TARGET)", "linux-x86")
    include ../../Makefile-include.linux-x86
  else ifeq ("$(TARGET)", "linux-x86_64")
    include ../../Makefile-include.linux-x86_64
  else ifeq ("$(TARGET)", "mikrotik-mips")
    include ../../Makefile-include.mikrotik-mips
  else ifeq ("$(TARGET)", "mikrotik-mipsel")
    include ../../Makefile-include.mikrotik-mipsel
  else ifeq ("$(TARGET)", "mikrotik-x86")
    include ../../Makefile-include.mikrotik-x86
  else ifeq ("$(TARGET)", "mikrotik-ppc")
    include ../../Makefile-include.mikrotik-ppc
  else ifeq ("$(TARGET)", "ubiquiti-mips")
    include ../../Makefile-include.ubiquiti-mips
  else ifeq ("$(TARGET)", "avtech-arm")
    include ../../Makefile-include.avtech-arm
  else ifeq ("$(TARGET)", "arm")
    include ../../Makefile-include.arm
  else ifeq ("$(TARGET)", "solaris-sparc")
    include ../../Makefile-include.solaris-sparc
  else ifeq ("$(TARGET)", "solaris-x86")
    include ../../Makefile-include.solaris-x86
  else
    TARGET:=
 endif
endif 
CFLAGS += -I../../debug -I../../crypto -I../include -I../ssl/include

ifdef DEBUG
  ifeq ("$(DEBUG)", "1")        
    CFLAGS += -g -O0
  endif
  ifeq ("$(DEBUG)", "2")        
    CFLAGS += -DDEBUG -D_DEBUG -g -O0
  endif
  ifeq ("$(DEBUG)", "3")        
    CFLAGS += -g -O0 -DDEBUG -D_DEBUG -DPOLARSSL_DEBUG_C -DPOLARSSL_DEBUG_MSG -DPOLARSSL_SSL_DEBUG_ALL -DPOLARSSL_SELF_TEST
  endif
else
  CFLAGS += -Os
endif

ifdef TARGET
  $(info =========================================================================)
  $(info TARGET: $(TARGET))
  $(info SRCDIR: $(PWD))
  $(info .   CC: $(CC))
  $(info CFLAGS: $(CFLAGS))
  $(info =========================================================================)
  $(info )
endif

# MicroBlaze specific options:
# CFLAGS += -mno-xl-soft-mul -mxl-barrel-shift

# To compile on Plan9:
# CFLAGS += -D_BSD_EXTENSION

# To compile as a shared library:
# CFLAGS += -fPIC

SONAME=libpolarssl.so.0


DLEXT=so
# OSX shared library extension:
# DLEXT=dylib

# Windows shared library extension:
# DLEXT=dll

OBJS=	aes.o		arc4.o		asn1parse.o	\
	base64.o	bignum.o	camellia.o	\
	certs.o		cipher.o	cipher_wrap.o	\
	ctr_drbg.o	debug.o		des.o		\
	dhm.o		entropy.o	entropy_poll.o	\
	error.o		havege.o			\
	md.o		md_wrap.o	md2.o		\
	md4.o		md5.o		net.o		\
	padlock.o	pem.o		pkcs11.o	\
	rsa.o		sha1.o		sha2.o		\
	sha4.o		ssl_cli.o	ssl_srv.o	\
	ssl_tls.o	timing.o	version.o	\
	x509parse.o	xtea.o

.SILENT:
.PHONY: $(SSL_BUILD_OPTS)

all: static

static: $(LIBDIR)/libpolarssl.a

shared: $(LIBDIR)/libpolarssl.$(DLEXT)

$(LIBDIR)/libpolarssl.a: $(SSL_BUILD_OPTS) $(OBJS)
	printf "\tAR\t$@\n"
	$(AR) $(ARFLAGS) $@ $(OBJS)
	echo "ARCH_BUILD_SSL=$(TARGET)" > ARCH_BUILD
	printf "=========================================================================\n"
	printf "\t$@ COMPLETE\n"
	printf "=========================================================================\n"

$(LIBDIR)/libpolarssl.so: libpolarssl.a
	printf "\tLD\t$@=\n"
	$(CC) ${LDFLAGS} -shared -Wl,-soname,$(SONAME) -o $@ $(OBJS)

.PHONY:	clean
clean:
	printf "\tCLEANING:\t$(CURDIR)\n"
	rm -f *.o
	>ARCH_BUILD

.PHONY:	distclean
distclean:	clean
	printf "\tDIST CLEANING:\t$(CURDIR)\n"
	rm -f libpolarssl.*

.c.o:
	printf "\tCC\t$<\n"
	$(CC) $(CFLAGS) -c $<
