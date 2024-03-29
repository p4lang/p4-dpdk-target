noinst_LTLIBRARIES =
lib_LTLIBRARIES =
SUBDIRS =
SUBDIRS += lld/dpdk

LIBPATH := $(shell gcc --print-multiarch)
LDFLAGS = -Wl,-rpath-link=$(prefix)/lib64 -Wl,-rpath-link=$(libdir)
LDFLAGS += -Wl,-rpath-link=$(prefix)/lib/$(LIBPATH)

bf_rt_shareddir = $(datadir)/bf_rt_shared
dist_bf_rt_shared_DATA =  \
bf_rt/bf_rt_port/dpdk/bf_rt_port.json

bfruntime_grpc_protocoldir = $(bf_rt_shareddir)/proto
dist_bfruntime_grpc_protocol_DATA = bf_rt/proto/bfruntime.proto

if IS_BFRT_ENABLED
bfrt_pythondir = $(libdir)/python3.10
bfrt_python_DATA = $(bfrt_python_python_sources) \
$(tdi_python_python_sources) \
$(tdi_rt_python_python_sources)

bfrt_python_python_sources = \
bf_rt/bf_rt_python/bfrtInfo.py \
bf_rt/bf_rt_python/bfrtTable.py \
bf_rt/bf_rt_python/bfrtTableEntry.py \
bf_rt/bf_rt_python/bfrtcli.py

tdi_python_python_sources = \
../third-party/tdi/tdi_python/tdicli.py \
../third-party/tdi/tdi_python/tdiTable.py \
../third-party/tdi/tdi_python/tdiTableEntry.py \
../third-party/tdi/tdi_python/tdiInfo.py \
../third-party/tdi/tdi_python/tdiDefs.py

tdi_rt_python_python_sources = \
tdi_rt/tdi_python/tdiRtCli.py \
tdi_rt/tdi_python/tdiRtDefs.py \
tdi_rt/tdi_python/tdiRtTable.py

endif # end IS_BFRT_ENABLED

libdriver_la_CFLAGS = $(AM_CFLAGS) $(BF_DRIVERS_CFLAGS) -Wno-absolute-value -Wno-deprecated-declarations
libdriver_la_CFLAGS += -I$(srcdir)/lld/dpdk
libdriver_la_CXXFLAGS = $(AM_CXXFLAGS) $(BF_DRIVERS_CXXFLAGS)
libdvm_la_CFLAGS = $(libdriver_la_CFLAGS)
liblld_la_CFLAGS = $(libdriver_la_CFLAGS) -fPIC -DPIC
liblld_la_LIBADD = -ldpdk_infra
libpipe_mgr_la_CFLAGS = $(libdriver_la_CFLAGS) -DPIPE_MGR_DEBUG
libport_mgr_la_CFLAGS = $(libdriver_la_CFLAGS)
libfixed_function_la_CFLAGS = $(libdriver_la_CFLAGS)
libbf_pm_la_CFLAGS = $(libdriver_la_CFLAGS)
libutils_la_CFLAGS = $(libdriver_la_CFLAGS)

if IS_BFRT_ENABLED
libbfrt_la_CFLAGS = $(libdriver_la_CFLAGS)
libbfrt_la_CXXFLAGS = $(libdriver_la_CXXFLAGS) -I$(srcdir)/bf_rt -I$(srcdir)/bf_rt/bf_rt_port/dpdk -I$(top_builddir)third_party/ptf_grpc \
                                               -I$(srcdir)/tdi_rt/tdi_port/dpdk
libtdirt_la_CFLAGS = $(libdriver_la_CFLAGS)
libtdirt_la_CXXFLAGS = $(libdriver_la_CXXFLAGS) -I$(srcdir)/tdi_rt/tdi_common \
						-I$(srcdir)/../third-party/tdi/include \
						-I$(srcdir)/bf_rt \
                                                -I$(srcdir)/tdi_rt \
                                                -I$(srcdir)/tdi_rt/tdi_port/dpdk
endif # end IS_BFRT_ENABLED


tdirt_sources = \
tdi_rt/tdi_common/tdi_rt_init.cpp \
tdi_rt/tdi_common/tdi_context_info.cpp \
tdi_rt/tdi_common/tdi_pipe_mgr_intf.cpp \
tdi_rt/tdi_common/tdi_fixed_mgr_intf.cpp \
tdi_rt/tdi_common/tdi_session_impl.cpp \
tdi_rt/tdi_common/tdi_rt_target.cpp \
tdi_rt/c_frontend/tdi_rt_init_c.cpp \
tdi_rt/tdi_port/tdi_port_table_key_impl.cpp \
tdi_rt/tdi_port/tdi_table_data_utils.cpp \
tdi_rt/tdi_p4/tdi_p4_table_impl.cpp \
tdi_rt/tdi_p4/tdi_p4_table_data_impl.cpp \
tdi_rt/tdi_p4/tdi_p4_table_key_impl.cpp \
tdi_rt/tdi_fixed/tdi_fixed_table_impl.cpp \
tdi_rt/tdi_fixed/tdi_fixed_table_data_impl.cpp \
tdi_rt/tdi_fixed/tdi_fixed_table_key_impl.cpp \
tdi_rt/tdi_common/tdi_table_attributes_impl.cpp

bfrt_sources = \
bf_rt/bf_rt_common/bf_rt_info_impl.cpp \
bf_rt/bf_rt_common/bf_rt_cjson.cpp \
bf_rt/bf_rt_common/bf_rt_table_data_impl.cpp \
bf_rt/bf_rt_common/bf_rt_table_key_impl.cpp \
bf_rt/bf_rt_common/bf_rt_init.cpp \
bf_rt/bf_rt_common/bf_rt_init_impl.cpp \
bf_rt/bf_rt_common/bf_rt_table_attributes_impl.cpp \
bf_rt/bf_rt_common/bf_rt_table_operations_impl.cpp \
bf_rt/bf_rt_common/bf_rt_table_data_utils.cpp \
bf_rt/bf_rt_common/bf_rt_utils.cpp \
bf_rt/bf_rt_common/bf_rt_table_impl.cpp \
bf_rt/bf_rt_common/bf_rt_session_impl.cpp \
bf_rt/bf_rt_common/bf_rt_pipe_mgr_intf.cpp \
bf_rt/bf_rt_p4/bf_rt_p4_table_data_impl.cpp \
bf_rt/bf_rt_p4/bf_rt_p4_table_impl.cpp \
bf_rt/bf_rt_p4/bf_rt_p4_table_key_impl.cpp \
bf_rt/bf_rt_p4/bf_rt_table_attributes_state.cpp \
bf_rt/bf_rt_p4/bf_rt_table_state.cpp \
bf_rt/bf_rt_port/bf_rt_port_mgr_intf.cpp \
bf_rt/bf_rt_port/bf_rt_port_table_key_impl.cpp \
bf_rt/bf_rt_port/bf_rt_port_table_attributes_state.cpp \
bf_rt/c_frontend/bf_rt_session_c.cpp \
bf_rt/c_frontend/bf_rt_table_c.cpp \
bf_rt/c_frontend/bf_rt_table_data_c.cpp \
bf_rt/c_frontend/bf_rt_table_key_c.cpp \
bf_rt/c_frontend/bf_rt_table_attributes_c.cpp \
bf_rt/c_frontend/bf_rt_table_operations_c.cpp \
bf_rt/c_frontend/bf_rt_state_c.cpp \
bf_rt/c_frontend/bf_rt_init_c.cpp \
bf_rt/c_frontend/bf_rt_info_c.cpp \
bf_rt/cli/bf_rt_cli.c

bfrt_sources += \
bf_rt/bf_rt_port/dpdk/bf_rt_port_table_impl.cpp \
bf_rt/bf_rt_port/dpdk/bf_rt_port_table_data_impl.cpp \
tdi_rt/tdi_port/dpdk/tdi_port_table_impl.cpp \
tdi_rt/tdi_port/dpdk/tdi_port_table_data_impl.cpp


bf_types_sources = \
bf_types/bf_types_log.h

dvm_sources = \
dvm/dvm.c \
dvm/dvm_clients.c \
dvm/bf_drv_shell.c

lld_sources = \
lld/lld.c \
lld/bf_lld_if.c \
lld/lld_debug.c

lld_sources += \
lld/dpdk/lld_dpdk_lib.h \
lld/dpdk/lld_dpdk_port.h \
lld/dpdk/lld_dpdk_lib.c \
lld/dpdk/lld_dpdk_dev.c \
lld/dpdk/lld_dpdk_port.c

port_mgr_sources = \
port_mgr/bf_port_if.c \
port_mgr/port_mgr.c \
port_mgr/port_mgr_dev.c \
port_mgr/port_mgr_fixed_function.c

port_mgr_sources += \
port_mgr/dal/dpdk/port_mgr_dpdk_port.c \
port_mgr/dal/dpdk/port_mgr_dpdk_config_json.c \
port_mgr/dal/dpdk/port_mgr_dpdk.c \
port_mgr/dal/dpdk/port_mgr_dpdk_hotplug.c \
port_mgr/dal/dpdk/dal_port_mgr.c

fixed_function_sources = \
fixed_function/fixed_function_init.h \
fixed_function/fixed_function_init.c \
fixed_function/fixed_function_ctx.h \
fixed_function/fixed_function_ctx.c \
fixed_function/fixed_function_int.h

bf_pm_sources = \
bf_pal/dev_intf.c \
bf_pal/bf_pal_port_intf.c

pipe_mgr_sources = \
pipe_mgr/core/pipe_mgr_log.h \
pipe_mgr/core/pipe_mgr_init.c \
pipe_mgr/core/pipe_mgr_ctx_json.c \
pipe_mgr/core/pipe_mgr_ucli.c \
pipe_mgr/shared/pipe_mgr_shared_intf.h \
pipe_mgr/shared/infra/pipe_mgr_int.h \
pipe_mgr/shared/infra/pipe_mgr_shared_init.c \
pipe_mgr/shared/infra/pipe_mgr_ctx_util.c \
pipe_mgr/shared/infra/pipe_mgr_ctx_util.h \
pipe_mgr/shared/infra/pipe_mgr_dbg.c \
pipe_mgr/shared/infra/pipe_mgr_dbg.h \
pipe_mgr/shared/infra/pipe_mgr_tbl.c \
pipe_mgr/shared/infra/pipe_mgr_tbl.h \
pipe_mgr/shared/infra/pipe_mgr_session.c \
pipe_mgr/shared/infra/pipe_mgr_session.h \
pipe_mgr/shared/features/pipe_mgr_value_lookup.c \
pipe_mgr/shared/features/pipe_mgr_counters.c \
pipe_mgr/shared/features/pipe_mgr_counters.h \
pipe_mgr/shared/features/pipe_mgr_registers.c \
pipe_mgr/shared/features/pipe_mgr_registers.h \
pipe_mgr/shared/features/pipe_mgr_mirror.c \
pipe_mgr/shared/features/pipe_mgr_mirror.h \
pipe_mgr/shared/features/pipe_mgr_mat.c \
pipe_mgr/shared/features/pipe_mgr_adt.c \
pipe_mgr/shared/features/pipe_mgr_sel.c \
pipe_mgr/shared/dal/dal_mat.h \
pipe_mgr/stubs/pipe_mgr_intf.c \
pipe_mgr/shared/dal/dal_init.h \
pipe_mgr/shared/dal/dal_value_lookup.h \
pipe_mgr/shared/dal/dal_counters.h \
pipe_mgr/shared/dal/dal_registers.h \
pipe_mgr/shared/dal/dal_mirror.h   \
pipe_mgr/shared/features/pipe_mgr_fixed.c \
pipe_mgr/shared/features/pipe_mgr_fixed.h

pipe_mgr_sources += \
pipe_mgr/shared/dal/dpdk/dal_mat.c \
pipe_mgr/shared/dal/dpdk/dal_adt.c \
pipe_mgr/shared/dal/dpdk/dal_sel.c \
pipe_mgr/shared/dal/dpdk/dal_tbl.h \
pipe_mgr/shared/dal/dpdk/dal_tbl.c \
pipe_mgr/shared/dal/dpdk/dal_init.c \
pipe_mgr/shared/dal/dpdk/dal_mirror.c \
pipe_mgr/shared/dal/dpdk/dal_counters.c \
pipe_mgr/shared/dal/dpdk/dal_registers.c \
pipe_mgr/shared/dal/dpdk/dal_value_lookup.c \
pipe_mgr/shared/dal/dpdk/pipe_mgr_dpdk_ctx_util.h \
pipe_mgr/shared/dal/dpdk/pipe_mgr_dpdk_ctx_util.c \
pipe_mgr/core/dal/dpdk/pipe_mgr_ctx_dpdk_json.c

proto_sources = \
bf_rt/proto/bfruntime.proto

proto_cpp_sources = \
bf_rt/cpp_out/bfruntime.pb.cc \
bf_rt/cpp_out/bfruntime.pb.h

proto_grpc_sources = \
bf_rt/cpp_out/bfruntime.grpc.pb.cc \
bf_rt/cpp_out/bfruntime.grpc.pb.h

proto_grpc_python_sources = \
bf_rt/py_out/__init__.py \
bf_rt/py_out/bfruntime_pb2_grpc.py \
bf_rt/py_out/bfruntime_pb2.py

bfruntime_grpc_python_sources = \
bf_rt/bfruntime_grpc_client/python/client.py \
bf_rt/bfruntime_grpc_client/python/info_parse.py

grpc_server_sources = \
bf_rt/proto/bf_rt_server_impl.cpp \
bf_rt/proto/bf_rt_server_mgr.cpp

ctx_json_sources = \
ctx_json/ctx_json_log.h \
ctx_json/ctx_json_utils.c

if IS_BFRT_ENABLED
libbfrt_la_SOURCES = $(bfrt_sources)
libtdirt_la_SOURCES = $(tdirt_sources)
endif
libdvm_la_SOURCES = $(dvm_sources)
liblld_la_SOURCES = $(lld_sources)
libpipe_mgr_la_SOURCES = $(pipe_mgr_sources)
libport_mgr_la_SOURCES = $(port_mgr_sources)
libfixed_function_la_SOURCES = $(fixed_function_sources)
libbf_pm_la_SOURCES = $(bf_pm_sources)
libutils_la_SOURCES = $(bf_types_sources)

if IS_GRPC_ENABLED
if IS_BFRT_ENABLED
# Two separate libraries for protobuf and grpc files. Similar to
# https://github.com/p4lang/PI/blob/master/proto/Makefile.am#L190-L196
# generated source should not be distributed
nodist_libbfrt_protobuf_la_SOURCES = $(proto_cpp_sources)
nodist_libbfrt_grpc_la_SOURCES = $(proto_grpc_sources)
libbfrt_grpc_server_la_SOURCES = $(grpc_server_sources)

libbfrt_grpc_la_CXXFLAGS = \
-I$(builddir)/bf_rt/cpp_out \
-I$(top_builddir)/third-party/grpc/cpp_out
libbfrt_protobuf_la_CXXFLAGS = \
-I$(builddir)/bf_rt/cpp_out \
-I$(top_builddir)/third-party/grpc/cpp_out
libbfrt_grpc_server_la_CXXFLAGS = \
-I$(builddir)/bf_rt/cpp_out \
-I$(top_builddir)/third-party/grpc/cpp_out \
-I$(srcdir)/bf_rt \
-I$(top_builddir)/third-party/ptf_grpc/protos \
-I$(top_builddir)/third-party/ptf_grpc/cpp_out

if IS_PTF_ENABLED
libbfrt_grpc_server_la_CXXFLAGS += \
-DPTF_ENABLED
endif

libbfrt_protobuf_la_LIBADD = \
$(top_builddir)/third-party/grpc/libbf_google_protobuf.la \
$(PROTOBUF_LIBS)
libbfrt_grpc_la_LIBADD = \
$(top_builddir)/third-party/grpc/libbf_google_grpc.la \
$(GRPC_LIBS)

noinst_LTLIBRARIES += \
libbfrt_protobuf.la \
libbfrt_grpc.la \
libbfrt_grpc_server.la

bfrt_grpcdir = $(pythondir)/bfrt_grpc
bfrt_grpc_PYTHON = $(proto_grpc_python_sources)

bfruntime_grpc_clientdir = $(pythondir)/bfrt_grpc
bfruntime_grpc_client_PYTHON = $(bfruntime_grpc_python_sources)
endif
endif

lib_LTLIBRARIES += libdriver.la

libdriver_la_LDFLAGS = -ltarget_sys -ltarget_utils -lstdc++ -lclish -lcjson

if IS_BFRT_ENABLED
libdriver_la_LDFLAGS += -lpython3.10 -ltdi -ltdi_pna 
else
if IS_DEBUG_PYTHON_SHELL_ENABLED
libdriver_la_LDFLAGS += -lpython3.10
endif #IS_DEBUG_PYTHON_SHELL_ENABLED
endif #IS_BFRT_ENABLED
if COVERAGE
libdriver_la_LDFLAGS += -lgcov
endif

noinst_LTLIBRARIES += \
libdvm.la \
liblld.la \
libpipe_mgr.la \
libport_mgr.la \
libfixed_function.la \
libbf_pm.la \
libutils.la

if IS_BFRT_ENABLED
noinst_LTLIBRARIES += \
libbfrt.la \
libtdirt.la
endif


libdriver_la_SOURCES = \
$(ctx_json_sources)
libdriver_la_LIBADD = \
libdvm.la \
liblld.la \
libpipe_mgr.la \
libbf_pm.la \
libport_mgr.la \
libfixed_function.la \
libutils.la

if IS_BFRT_ENABLED
libdriver_la_LIBADD += libbfrt.la libtdirt.la
endif

if IS_BFRT_ENABLED
if IS_GRPC_ENABLED
libdriver_la_LIBADD += \
libbfrt_protobuf.la \
libbfrt_grpc.la \
libbfrt_grpc_server.la

if IS_PTF_ENABLED
libdriver_la_LIBADD += \
$(top_builddir)/third-party/ptf_grpc/libptf_grpc.la \
$(top_builddir)/third-party/ptf_grpc/libptf_protobuf.la \
$(top_builddir)/third-party/ptf_grpc/libptf_grpc_server.la
endif

PROTOFLAGS = -I$(srcdir)/bf_rt/proto -I$(top_srcdir)/third-party/grpc

BUILT_SOURCES = $(proto_cpp_sources) $(proto_grpc_sources) \
$(proto_grpc_python_sources)

# See http://www.gnu.org/software/automake/manual/html_node/Multiple-Outputs.html
bfruntime_proto.ts: $(proto_sources)
	@rm -f bfruntime_proto.tmp
	@touch bfruntime_proto.tmp
	@mkdir -p $(builddir)/bf_rt/cpp_out
	@mkdir -p $(builddir)/bf_rt/py_out
	$(PROTOC) $^ --cpp_out $(builddir)/bf_rt/cpp_out $(PROTOFLAGS)
	$(PROTOC) $^ --grpc_out $(builddir)/bf_rt/cpp_out \
		--plugin=protoc-gen-grpc=$(GRPC_CPP_PLUGIN) $(PROTOFLAGS)
	$(PROTOC) $^ --python_out $(builddir)/bf_rt/py_out \
		--grpc_out $(builddir)/bf_rt/py_out \
		--plugin=protoc-gen-grpc=$(GRPC_PY_PLUGIN) $(PROTOFLAGS)
	@touch $(builddir)/bf_rt/py_out/__init__.py
# hack to convert from python 2 to 3
	@python -m lib2to3 -w -n $(builddir)/bf_rt/py_out/*_pb2*.py
	@mv -f bfruntime_proto.tmp $@

$(BUILT_SOURCES): bfruntime_proto.ts
## Recover from the removal of $@
	@if test -f $@; then :; else \
	  trap 'rm -rf bfruntime_proto.lock bfruntime_proto.ts' 1 2 13 15; \
## mkdir is a portable test-and-set
	if mkdir bfruntime_proto.lock 2>/dev/null; then \
## This code is being executed by the first process.
	  rm -f bfruntime_proto.ts; \
	  $(MAKE) $(AM_MAKEFLAGS) bfruntime_proto.ts; \
	  result=$$?; rm -rf bfruntime_proto.lock; exit $$result; \
	else \
## This code is being executed by the follower processes.
## Wait until the first process is done.
	  while test -d bfruntime_proto.lock; do sleep 1; done; \
## Succeed if and only if the first process succeeded.
	    test -f bfruntime_proto.ts; \
	  fi; \
	fi

endif #IS_GRPC_ENABLED

bf_rt_xmldir = $(datadir)/cli/xml
bf_rt_xml_DATA = bf_rt/xml/bf_rt.xml
bf_rt_xml_DATA += tdi_rt/xml/tdi.xml

lib_LTLIBRARIES += bfshell_plugin_bf_rt.la
bfshell_plugin_bf_rt_la_CFLAGS = $(AM_CFLAGS) $(BF_DRIVERS_CFLAGS)
bfshell_plugin_bf_rt_la_LDFLAGS = -module
bfshell_plugin_bf_rt_la_SOURCES = bf_rt/cli/bf_rt_cli.c

lib_LTLIBRARIES += bfshell_plugin_tdi.la
bfshell_plugin_tdi_la_CFLAGS = $(AM_CFLAGS) $(BF_DRIVERS_CFLAGS)
bfshell_plugin_tdi_la_LDFLAGS = -module
bfshell_plugin_tdi_la_SOURCES = tdi_rt/cli/tdi_cli.c

endif #IS_BFRT_ENABLED

pipemgr_xmldir = $(datadir)/cli/xml
pipemgr_xml_DATA = pipe_mgr/xml/pipemgr.xml

lib_LTLIBRARIES += bfshell_plugin_pipemgr.la
bfshell_plugin_pipemgr_la_CFLAGS = $(AM_CFLAGS) $(BF_DRIVERS_CFLAGS)
bfshell_plugin_pipemgr_la_LDFLAGS = -module
bfshell_plugin_pipemgr_la_SOURCES = pipe_mgr/core/pipe_mgr_cli.c

if IS_DEBUG_PYTHON_SHELL_ENABLED
debug_xmldir = $(datadir)/cli/xml
debug_xml_DATA = lld/cli/xml/debug.xml

lib_LTLIBRARIES += bfshell_plugin_debug.la
bfshell_plugin_debug_la_CFLAGS = $(AM_CFLAGS) $(BF_DRIVERS_CFLAGS)
bfshell_plugin_debug_la_LDFLAGS = -module
bfshell_plugin_debug_la_SOURCES = lld/cli/debug_cli.c
endif #IS_DEBUG_PYTHON_SHELL_ENABLED
