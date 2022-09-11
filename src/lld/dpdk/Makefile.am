all:
	$(shell ./apply_patch.sh > /dev/null 2>&1)
	stat dpdk_src/build > /dev/null 2>&1 || \
        (cd dpdk_src && meson -Dprefix=$(prefix) -Dtests=false -Ddefault_library=shared \
	 -Dexamples='' -Denable_kmods=false -Denable_docs=false -Denable_driver_sdk=false \
	 -Ddisable_drivers=dma/*,raw/*,compress/*,crypto/*,event/*,baseband/* -Dbuildtype=release build && \
	cd build && meson compile -j4 && meson install)
	$(MAKE) -C infra install_dir=$(prefix)
	cp infra/build/dpdk_infra.so $(libdir)/libdpdk_infra.so
install:
	echo "Nothing to be done in install"
uninstall:
	rm -rf $(libdir)/libdpdk_infra.so
	rm -rf $(libdir)/librte*
clean:
	cd dpdk_src && rm -rf build
	$(MAKE) -C infra clean
