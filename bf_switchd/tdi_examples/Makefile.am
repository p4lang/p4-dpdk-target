all:
	mkdir -p build
	cd build && cmake -DCMAKE_INSTALL_PREFIX=$(prefix) ..
	$(MAKE) -C build
	$(MAKE) -C build install
install:
	echo "Nothing to be done in install for tdi_examples (already done)"
clean:
	rm -rf build
