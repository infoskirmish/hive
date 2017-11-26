CLIENT = hclient-$(TARGET)

COMMON = ../common
CRYPTO = $(COMMON)/crypto
SSL = $(COMMON)/ssl

.SILENT:
.PHONY: all
all: 
	printf "\n\nPlease specify a supported target:\n\n"
	printf "\tmake linux-x86\n"
#	printf "\tmake linux-x86_64\n"
	printf "\nFor a debug version use:\n"
	printf "\n\tmake DEBUG=1 linux-x86\n"
	printf "\n\tAdministrative tasks, ...\n"
	printf "\tmake clean\n"

linux-x86:
	make -f Makefile.arch TARGET=linux-x86

linux-x86_64:
	make -f Makefile.arch TARGET=linux-x86_64

library: clean
	make -f Makefile.arch TARGET=$(TARGET) $@

.PHONY: clean
clean:
	make -f Makefile.arch $@

.PHONY: dist-clean
dist-clean:	clean
	echo
	rm -f $(CLIENT)*
