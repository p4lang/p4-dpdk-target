ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS} -I m4

SUBDIRS = .

noinst_LTLIBRARIES=
EXTRA_DIST=

if IS_GRPC_ENABLED

proto_sources = \
google/rpc/status.proto \
google/rpc/code.proto

proto_cpp_sources = \
cpp_out/google/rpc/status.pb.cc \
cpp_out/google/rpc/status.pb.h \
cpp_out/google/rpc/code.pb.cc \
cpp_out/google/rpc/code.pb.h

proto_grpc_sources = \
cpp_out/google/rpc/status.grpc.pb.cc \
cpp_out/google/rpc/status.grpc.pb.h \
cpp_out/google/rpc/code.grpc.pb.cc \
cpp_out/google/rpc/code.grpc.pb.h

proto_google_python_sources = \
py_out/google/__init__.py

proto_google_rpc_python_sources = \
py_out/google/rpc/__init__.py \
py_out/google/rpc/status_pb2_grpc.py \
py_out/google/rpc/status_pb2.py \
py_out/google/rpc/code_pb2_grpc.py \
py_out/google/rpc/code_pb2.py

googlepydir = $(pythondir)/tofino/google
nodist_googlepy_PYTHON = $(proto_google_python_sources)

googlerpcpydir = $(pythondir)/tofino/google/rpc
nodist_googlerpcpy_PYTHON = $(proto_google_rpc_python_sources)

PROTOFLAGS = -I$(srcdir)

BUILT_SOURCES = $(proto_cpp_sources) $(proto_grpc_sources) \
$(proto_google_python_sources) $(proto_google_rpc_python_sources)

# See http://www.gnu.org/software/automake/manual/html_node/Multiple-Outputs.html
proto_files.ts: $(proto_sources)
	@rm -f proto_files.tmp
	@touch proto_files.tmp
	@mkdir -p $(builddir)/cpp_out
	@mkdir -p $(builddir)/py_out
	$(PROTOC) $^ --cpp_out $(builddir)/cpp_out \
	    --grpc_out $(builddir)/cpp_out \
            --plugin=protoc-gen-grpc=$(GRPC_CPP_PLUGIN) $(PROTOFLAGS)
	$(PROTOC) $^ --python_out $(builddir)/py_out \
            --grpc_out $(builddir)/py_out \
	    --plugin=protoc-gen-grpc=$(GRPC_PY_PLUGIN) $(PROTOFLAGS)
	@touch $(builddir)/py_out/google/__init__.py
	@touch $(builddir)/py_out/google/rpc/__init__.py
	@mv -f proto_files.tmp $@

$(BUILT_SOURCES): proto_files.ts
## Recover from the removal of $@
	@if test -f $@; then :; else \
	  trap 'rm -rf proto_files.lock proto_files.ts' 1 2 13 15; \
## mkdir is a portable test-and-set
	if mkdir proto_files.lock 2>/dev/null; then \
## This code is being executed by the first process.
	  rm -f proto_files.ts; \
	  $(MAKE) $(AM_MAKEFLAGS) proto_files.ts; \
	  result=$$?; rm -rf proto_files.lock; exit $$result; \
	else \
## This code is being executed by the follower processes.
## Wait until the first process is done.
	  while test -d proto_files.lock; do sleep 1; done; \
## Succeed if and only if the first process succeeded.
	    test -f proto_files.ts; \
	  fi; \
	fi

noinst_LTLIBRARIES += \
libbf_google_protobuf.la \
libbf_google_grpc.la

nodist_libbf_google_protobuf_la_SOURCES = $(proto_cpp_sources)
nodist_libbf_google_grpc_la_SOURCES = $(proto_grpc_sources)

libbf_google_protobuf_la_LIBADD = $(PROTOBUF_LIBS)
libbf_google_grpc_la_LIBADD = $(PROTOBUF_LIBS) $(GRPC_LIBS)

libbf_google_protobuf_la_CPPFLAGS = $(AM_CPPFLAGS) -I$(builddir)/cpp_out
libbf_google_grpc_la_CPPFLAGS = $(AM_CPPFLAGS) -I$(builddir)/cpp_out

# These header files are used by P4Runtime and need to be installed
includegoogledir = $(includedir)/google/rpc/
nodist_includegoogle_HEADERS = \
cpp_out/google/rpc/status.pb.h \
cpp_out/google/rpc/code.pb.h \
cpp_out/google/rpc/status.grpc.pb.h \
cpp_out/google/rpc/code.grpc.pb.h

CLEANFILES = $(BUILT_SOURCES) proto_files.ts

endif  # IS_GRPC_ENABLED
