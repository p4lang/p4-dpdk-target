all:
	$(shell patch -dtarget-utils --no-backup-if-mismatch -p1 < build_fix_target_utils.patch > /dev/null 2>&1)
	mkdir -p target-syslibs/build
	mkdir -p target-utils/build
	mkdir -p googletest/build
	cd target-syslibs/build && cmake -DCMAKE_INSTALL_PREFIX=$(prefix) ..
	cd target-utils/build && cmake -DCMAKE_INSTALL_PREFIX=$(prefix) -DCPYTHON=1 -DSTANDALONE=ON ..
	cd googletest/build && cmake -DCMAKE_INSTALL_PREFIX=$(prefix) ..
	$(MAKE) -C target-syslibs/build -j
	$(MAKE) -C target-utils/build -j
	$(MAKE) -C target-utils/build install
	$(MAKE) -C googletest/build -j 
	$(MAKE) -C googletest/build install
install:
	echo "Nothing to be done in install"
clean:
	$(shell patch -dtarget-utils --no-backup-if-mismatch -p1 -Rfs -r - < build_fix_target_utils.patch > /dev/null 2>&1)
	rm -rf target-syslibs/build
	rm -rf target-utils/build
	rm -rf googletest/build
