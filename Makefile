.SILENT:
all:
	@echo
	@echo " Options:"
	@echo "  .  make clean"
	@echo "  .  make tarball"
	@echo "  .  make patcher"
	@echo
	
.PHONY: clean
clean:
	make -C server clean
	make -C client clean
	rm -rf *.tar Logs

.PHONY: tarball
tarball:
	printf "\nCreating $@ ...\n"
	tar	--exclude=.svn \
		--exclude=HiveServer.sdf \
		--exclude=client/patchedHives/* \
		--exclude=client/hived-*-upatched \
		--exclude=client/hclient--*-upatched \
		--exclude=server/hived-* \
		--exclude=*.gz \
		--exclude=*.tar \
		--exclude=*.tgz \
		--exclude=*.o \
		--exclude=*.a \
		--exclude=*.md5 \
		--exclude=documentation/html/* \
		--exclude=snapshot_* \
		--exclude=.*.swp \
		--exclude=ARCH_BUILD \
		-cvf hive.tar * >/dev/null

.PHONY: ilm-tar
ilm-tar:
	tar --exclude .svn --exclude HiveServer.sdf --exclude *.gz --exclude *.tar --exclude *.tgz -czvf hive-ilm-1.1.tgz client/ libs/ ilm-client/

.PHONY: patcher
patcher:
	printf "\n\nRun the Hive patcher only on hive-builder\n\n"
	sleep 2
	cd server && make linux-x86
	cd server && make mikrotik-x86
	cd server && make mikrotik-ppc
	cd server && make mikrotik-mips
	cd server && make mikrotik-mipsel
	cd client && make clean && make patcher

.PHONY: linux-x86
linux-x86:
	@make -C server $@
	@echo $@

.PHONY: deliverables
deliverables:	remove-deliverables tarball
	printf "Packaging Deliverables, please wait ...\n"
	mkdir -p deliverables/BIN
	mkdir -p deliverables/DOC
	mkdir -p deliverables/SRC
	mkdir -p deliverables/OTHER
	bzip2 -fc hive.tar > deliverables/SRC/hive.tar.bz2
	cp -a ilm-client/CCS.xml* deliverables/BIN
	cp -a ilm-client/cutthroat* deliverables/BIN
	cp -a ilm-client/hive deliverables/BIN
	cp -a ilm-client/hive.md5 deliverables/BIN
	cp -a client/hive-patcher deliverables/BIN
	cp -a client/hive-patcher.md5 deliverables/BIN
	cp -a ilm-client/resetTimer_v1.0/hiveReset_v1_0.py deliverables/BIN
	cp -a ilm-client/server.key deliverables/BIN
	cp -a ilm-client/server.crt deliverables/BIN
	cp -a ilm-client/ca.crt deliverables/BIN
	cp -a honeycomb/honeycomb.py deliverables/BIN
	md5sum honeycomb/honeycomb.py > deliverables/BIN/honeycomb.py.md5
	mkdir -p deliverables/BIN/unpatched
	cp -aL client/hived-*-*-unpatched deliverables/BIN/unpatched
	(cd deliverables/BIN/unpatched;for i in *; do md5sum $$i > $$i.md5;done)
	cp -a documentation/UsersGuide/* deliverables/DOC/

.PHONY: remove-deliverables
remove-deliverables:
	rm -rf deliverables
