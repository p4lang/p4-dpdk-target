all:
	mkdir -p tdi/third-party/googletest/build
	mkdir -p tdi/build
	cd tdi/third-party/googletest/build && cmake -DCMAKE_INSTALL_PREFIX=$(prefix) ..
	cd tdi/build && cmake -DCMAKE_INSTALL_PREFIX=$(prefix) -DCPYTHON=1 -DSTANDALONE=ON -DTARGET_CPLIB=${target_cplib} -DENABLE_PARALLEL_COMPILE=${ENABLE_PARALLEL_COMPILE} ..
	$(MAKE) -C tdi/third-party/googletest/build $(ENABLE_PARALLEL_COMPILE)
	$(MAKE) -C tdi/third-party/googletest/build install
	$(MAKE) -C tdi/build $(ENABLE_PARALLEL_COMPILE)
	$(MAKE) -C tdi/build install
install:
	echo "Nothing to be done in install"
clean:
	rm -rf tdi/third-party/googletest/build
	rm -rf tdi/build
