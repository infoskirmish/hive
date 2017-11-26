#-------------------------------------------------------------------------------
# Since we are building Hive for multiple architectures we have
# adopted the following standards...
#  Makfile naming format = Makefile.targetOS-architecture
#  
#  Existing Makefiles include:
#
#	Makefile.linux-x86
#	Makefile.solaris-x86
#	Makefile.mikrotik-x86
#	Makefile.mikrotik-mipsbe
#	Makefile.mikrotik-mipsle
#	Makefile.mikrotik-ppc
#
#   Another file named "Makefile" will contain targets for each of
#   these Makefiles.
#
#-------------------------------------------------------------------------------

CLIENT_BASENAME = hive
SERVER_BASENAME = hived

COMMON = ../common
SSL = $(COMMON)/ssl/library
CRYPTO = $(COMMON)/crypto
BZIP2 = $(COMMON)/bzip2
STRINGS = $(COMMON)/strings
LIBDEBUG = $(COMMON)/debug

.SILENT:

.PHONY: all linux-x86
all: 
	printf "\n\nPlease specify a supported target:\n\n"
	printf "\tmake linux-x86\n"
	printf "\tmake linux-x86_64\n"
#	printf "\tmake solaris-x86\n"
	printf "\tmake mikrotik-x86\n"
	printf "\tmake mikrotik-mips\n"
#	printf "\tmake mikrotik-mipsel\n"
	printf "\tmake mikrotik-ppc\n"
	printf "\tmake ubiquiti-mips\n"
	printf "\tmake avtech-arm\n\n"
	printf "To build a debug version use:\n"
	printf "\t make DEBUG=1 <target>\t\tFor symbols only\n"
	printf "\t make DEBUG=2 <target>\t\tHive debug code\n"
	printf "\t make DEBUG=3 <target>\t\tHive and  PolarSSL debug code\n\n"
	printf "Administrative tasks, ...\n"
	printf "\tmake clean\n"
	printf "\tmake tarball    ( on Linux machines only)\n\n"

linux-x86:
	@make -f Makefile.arch TARGET=linux-x86

linux-x86_64:
	@make -f Makefile.arch TARGET=linux-x86_64

#solaris-x86:
#	@make -f Makefile.arch TARGET=solaris-x86

mikrotik-x86:
	@make -f Makefile.arch TARGET=mikrotik-x86

#mikrotik-mipsel:
#	@make -f Makefile.arch TARGET=mikrotik-mipsel

mikrotik-mips:
	@make -f Makefile.arch TARGET=mikrotik-mips

mikrotik-ppc:
	@make -f Makefile.arch TARGET=mikrotik-ppc

ubiquiti-mips:
	@make -f Makefile.arch TARGET=ubiquiti-mips

avtech-arm:
	@make -f Makefile.arch TARGET=avtech-arm

strings:
	rm -f proj_strings.h
	@make -f Makefile.arch proj_strings.h

CLEAN_LIST := *.o *PATCHED init_strings.* proj_strings* init_crypto_strings.*
CLEAN_LIST += crypto_proj_strings.h crypto_strings_main.h string_utils.h.gch
CLEAN_LIST += mod_hexify.pyc ../hive-server.tar

.PHONY: build_clean
build_clean:
	printf "\n\tCLEANING:\t$(CURDIR)\n"
	rm -f *.o init_strings.* init_crypto_strings.* 

.PHONY: clean
clean:
	printf "\tCLEANING:\t$(CURDIR)\n"
	for file in $(CLEAN_LIST); do \
		rm -f $$file; \
	done
	>ARCH_BUILD

	$(MAKE) -C $(STRINGS) clean
	$(MAKE) -C $(SSL) clean
	$(MAKE) -C $(CRYPTO) clean
	$(MAKE) -C $(BZIP2) clean
	$(MAKE) -C $(LIBDEBUG) clean

.PHONY: distclean
distclean: build_clean
	printf "\tDIST CLEANING:\t$(CURDIR)\n"
	rm -f $(SERVER_BASENAME)-*
	$(MAKE) -C $(SSL) $@
	$(MAKE) -C $(CRYPTO) $@
	$(MAKE) -C $(BZIP2) $@
	$(MAKE) -C $(LIBDEBUG) $@

tarball: clean
	cd .. && tar \
		--exclude=.svn \
		--exclude=*.gz \
		--exclude=*.tar \
		--exclude=*.tgz \
		--exclude=documentation/html \
		--exclude=hived-* \
		-cvf hive-server.tar server common
