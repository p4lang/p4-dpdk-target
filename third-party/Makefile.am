all:
	mkdir -p tdi/build
	cd tdi/build && cmake -DCMAKE_INSTALL_PREFIX=$(prefix) -DCPYTHON=1 -DSTANDALONE=ON ..
	$(MAKE) -C tdi/build -j
	$(MAKE) -C tdi/build install
	mkdir -p tdi/third-party/googletest/build
	cd tdi/third-party/googletest/build && cmake -DCMAKE_INSTALL_PREFIX=$(prefix) ..
	$(MAKE) -C tdi/third-party/googletest/build -j 
	$(MAKE) -C tdi/third-party/googletest/build install
install:
	echo "Nothing to be done in install"
clean:
	rm -rf tdi/build
	rm -rf tdi/third-party/googletest/build
