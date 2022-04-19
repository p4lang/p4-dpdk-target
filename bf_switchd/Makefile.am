bfswitchd_includedir = $(includedir)/bf_switchd
nobase_bfswitchd_include_HEADERS = bf_switchd.h lib/bf_switchd_lib_init.h

bf_switchd_datadir = $(datadir)/bf_switchd
libbf_switchd_lib_la_CFLAGS = $(AM_CFLAGS) $(BF_DRIVERS_CFLAGS)
libbf_switchd_lib_la_CPPFLAGS = $(AM_CPPFLAGS) -I$(top_srcdir)
libbf_switchd_lib_la_LDFLAGS = -L$(top_builddir)/src $(AM_LDFLAGS)
libbf_switchd_lib_la_LIBADD =

bf_switchd_CFLAGS =
bf_switchd_CPPFLAGS =
bf_switchd_LDFLAGS =
bf_switchd_LDADD =

bf_switchd_lib_sources = \
bf_switchd.c \
switch_config.c \
switchd_ucli.c \
switch_config.h

if IS_PTF_ENABLED
bf_switchd_lib_sources += \
bf_switchd_ptf.c
endif
export top_srcdir libdir prefix

libbf_switchd_lib_la_SOURCES = $(bf_switchd_lib_sources)
lib_LTLIBRARIES = libbf_switchd_lib.la

if IS_GRPC_ENABLED
if !IS_STATIC_LINKED_LIB
bf_switchd_CPPFLAGS += -DGRPC_ENABLED
endif
libbf_switchd_lib_la_CPPFLAGS += -DGRPC_ENABLED
endif

if IS_BFRT_ENABLED
if !IS_STATIC_LINKED_LIB
bf_switchd_CPPFLAGS += -DBFRT_ENABLED
endif
libbf_switchd_lib_la_CPPFLAGS += -DBFRT_ENABLED
endif

if IS_STATIC_LINKED_LIB
libbf_switchd_lib_la_CPPFLAGS += -DSTATIC_LINK_LIB
endif

if COVERAGE
libbf_switchd_lib_la_CFLAGS += -DCOVERAGE_ENABLED
libbf_switchd_lib_la_CPPFLAGS += -DCOVERAGE_ENABLED
if !IS_STATIC_LINKED_LIB
bf_switchd_CPPFLAGS += -DCOVERAGE_ENABLED
bf_switchd_LDFLAGS += -lgcov
endif
endif

dpdk_lib != find $(prefix) -name librte_eal.so | xargs dirname
libbf_switchd_lib_la_LDFLAGS += -Wl,-rpath-link,$(dpdk_lib)
libbf_switchd_lib_la_LIBADD += -ltarget_sys -ltarget_utils -ldriver -lstdc++ -ldpdk_infra -lclish -lcjson

if !IS_STATIC_LINKED_LIB
bf_switchd_DEPENDENCIES = libbf_switchd_lib.la
libbf_switchd_lib_la_SOURCES += lib/bf_switchd_lib_init.c
bin_PROGRAMS = bf_switchd
bf_switchd_CFLAGS += $(AM_CFLAGS) $(BF_DRIVERS_CFLAGS)
bf_switchd_CPPFLAGS += $(AM_CPPFLAGS)

bf_switchd_SOURCES = \
bf_switchd_main.c

bf_switchd_LDFLAGS += -Wl,-rpath-link,$(dpdk_lib) -L$(top_builddir)/src -L$(top_builddir)/bf_switchd \
			$(AM_LDFLAGS)
bf_switchd_LDADD += -lbf_switchd_lib
endif
