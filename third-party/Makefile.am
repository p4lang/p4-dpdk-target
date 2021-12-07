all:
	mkdir -p target-syslibs/build
	mkdir -p target-utils/build
	cd target-syslibs/build && cmake -DCMAKE_INSTALL_PREFIX=$(prefix) ..
	cd target-utils/build && cmake -DCMAKE_INSTALL_PREFIX=$(prefix) -DCPYTHON=1 -DSTANDALONE=ON ..
	$(MAKE) -C target-syslibs/build -j
	$(MAKE) -C target-utils/build -j
	$(MAKE) -C target-utils/build install
install:
	echo "Nothing to be done in install"
clean:
	rm -rf target-syslibs/build
	rm -rf target-utils/build
