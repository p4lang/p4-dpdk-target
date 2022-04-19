ACLOCAL_AMFLAGS = -I m4

# ptf-utils

ptfutilspythondir = ${pythondir}/p4testutils
ptfutilspython_PYTHON = \
@srcdir@/ptf-utils/bf_switchd_dev_status.py \
@srcdir@/ptf-utils/ptf_interfaces.py \
@srcdir@/ptf-utils/run_ptf_tests.py \
@srcdir@/ptf-utils/gen_conf.py

scriptsdir = @bindir@
scripts_DATA = \
@srcdir@/ptf/ptf \
@srcdir@/ptf-utils/run_p4_tests.sh \
@srcdir@/ptf-utils/run_ptf_test_suite.sh \
@srcdir@/ptf-utils/port_mapping_clean \
@srcdir@/ptf-utils/port_mapping_setup \
@srcdir@/ptf-utils/port_ifup \
@srcdir@/ptf-utils/port_ifdown \
@srcdir@/ptf-utils/veth_setup.sh \
@srcdir@/ptf-utils/veth_teardown.sh \
@srcdir@/ptf-utils/dma_setup.sh \
@srcdir@/ptf-utils/gen_p4_artifacts.sh


noinst_LTLIBRARIES =
#lib_LTLIBRARIES =

ptf_shareddir = $(datadir)/ptf_shared
ptf_grpc_protocoldir =  $(ptf_shareddir)/protos
dist_ptf_grpc_protocol_DATA = protos/ptfRpc.proto

proto_sources = \
protos/ptfRpc.proto

proto_cpp_sources = \
cpp_out/ptfRpc.pb.cc \
cpp_out/ptfRpc.pb.h

proto_grpc_sources = \
cpp_out/ptfRpc.grpc.pb.cc \
cpp_out/ptfRpc.grpc.pb.h

proto_grpc_python_sources = \
py_out/__init__.py \
py_out/ptfRpc_pb2_grpc.py \
py_out/ptfRpc_pb2.py

ptf_grpc_server_sources = \
protos/ptf_server_impl.cpp

if IS_GRPC_ENABLED

nodist_libptf_protobuf_la_SOURCES = $(proto_cpp_sources)
nodist_libptf_grpc_la_SOURCES = $(proto_grpc_sources)
libptf_grpc_server_la_SOURCES = $(ptf_grpc_server_sources)

libptf_grpc_la_CXXFLAGS = \
-I$(builddir)/cpp_out

libptf_protobuf_la_CXXFLAGS = \
-I$(builddir)/cpp_out

libptf_grpc_server_la_CXXFLAGS = \
-I$(builddir)/cpp_out

noinst_LTLIBRARIES += \
libptf_grpc.la \
libptf_protobuf.la \
libptf_grpc_server.la

ptf_grpcdir = $(pythondir)/ptf_grpc
ptf_grpc_PYTHON = $(proto_grpc_python_sources)

PROTOFLAGS = -I$(builddir)/protos

BUILT_SOURCES = $(proto_cpp_sources) $(proto_grpc_sources) \
$(proto_grpc_python_sources)

ptfRpc_proto.ts : $(proto_sources)
	@rm -f ptfRpc_proto.tmp
	@touch ptfRpc_proto.tmp
	@mkdir -p $(builddir)/cpp_out
	@mkdir -p $(builddir)/py_out
	$(PROTOC) $^ --cpp_out $(builddir)/cpp_out $(PROTOFLAGS)
	$(PROTOC) $^ --grpc_out $(builddir)/cpp_out \
		--plugin=protoc-gen-grpc=$(GRPC_CPP_PLUGIN) $(PROTOFLAGS)
	$(PROTOC) $^ --python_out $(builddir)/py_out \
		--grpc_out $(builddir)/py_out \
		--plugin=protoc-gen-grpc=$(GRPC_PY_PLUGIN) $(PROTOFLAGS)
	@touch $(builddir)/py_out/__init__.py
# hack to convert from python 2 to 3
	@python -m lib2to3 -w -n $(builddir)/py_out/*_pb2*.py
	@mv -f ptfRpc_proto.tmp $@

$(BUILT_SOURCES): ptfRpc_proto.ts
## Recover from the removal of $@
	@if test -f $@; then :; else \
		trap 'rm -rf ptfRpc_proto.lock ptfRpc_proto.ts' 1 2 13 15; \
## mkdir is a portable test-and-set
	if mkdir ptfRpc_proto.lock 2>/dev/null; then \
## This code is being executed by the first process.
	rm -f ptfRpc_proto.ts; \
	$(MAKE) $(AM_MAKEFLAGS) ptfRpc_proto.ts; \
	result=$$?; rm -rf ptfRpc_proto.lock; exit $$result; \
	else \
## This code is being executed by the follower processes.
## Wait until the first process is done.
	while test -d ptfRpc_proto.lock; do sleep 1; done; \
## Succeed if and only if the first process succeeded.
		test -f bfruntime_proto.ts; \
	 fi; \
	fi

endif #IS_GRPC_ENABLED

# Create a soft-link to run_tests.py for backward compatibility.

install-data-hook:
	chmod +x @bindir@/run_p4_tests.sh
	chmod +x @bindir@/run_ptf_test_suite.sh
	chmod +x @bindir@/ptf
	chmod +x @bindir@/port_mapping_clean
	chmod +x @bindir@/port_mapping_setup
	chmod +x @bindir@/port_ifup
	chmod +x @bindir@/port_ifdown
	chmod +x @bindir@/veth_setup.sh
	chmod +x @bindir@/veth_teardown.sh
	chmod +x @bindir@/dma_setup.sh
	chmod +x @bindir@/gen_p4_artifacts.sh

p4_target-install-local: install
