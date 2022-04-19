#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([bfdrivers], [0.1], [])
AM_INIT_AUTOMAKE([foreign subdir-objects])
AM_PATH_PYTHON([2.7],,[AM_PATH_PYTHON([3.0],,[AC_MSG_ERROR([Python 2.7 or 3.x not found])])])
AM_EXTRA_RECURSIVE_TARGETS([libmodel])
AM_EXTRA_RECURSIVE_TARGETS([doc])
AM_SILENT_RULES([yes])
AC_CONFIG_HEADERS([config.h])

# Option to build for Unit-Test
AC_ARG_ENABLE([unittest],
              AS_HELP_STRING([--enable-unittest], ["Build with Unit Test support, default=no"])),
AM_CONDITIONAL([IS_UT_ENABLED],[test "x$enable_unittest" = "xyes"])

#Option to build with PTF
AC_ARG_ENABLE([ptf],
	      AS_HELP_STRING([--enable-ptf], ["Build with ptf test support, default=no"]))
AM_CONDITIONAL([IS_PTF_ENABLED],[test "x$enable_ptf" = "xyes"])

# Option to compile with ASAN checks
AC_ARG_WITH([asan],
            [AS_HELP_STRING([--with-asan],
                            [Build with address sanitizer checking])],
            [with_asan=${withval}],
            [with_asan=no])
AS_IF([test "x$with_asan" = "xno"],
      [ASAN_FLAGS=],
      [test "x$with_asan" = "xyes"],
      [ASAN_FLAGS="-fsanitize=address -fsanitize-recover=address"],
      [ASAN_FLAGS="$with_asan"])

# Option to compile C API with generic flags
AC_ARG_WITH([generic-flags],
            [AS_HELP_STRING([--with-generic-flags],
                            [Build with generic flags C API])],
            [with_generic_flags=${withval}],
            [with_generic_flags=no])
AS_IF([test "x$with_generic_flags" = "xno"],
      [GENERIC_FLAGS=],
      [test "x$with_generic_flags" = "xyes"],
      [GENERIC_FLAGS="-DBFRT_GENERIC_FLAGS"],
      [GENERIC_FLAGS="$with_generic_flags"])

AC_ARG_ENABLE([coverage],
    AS_HELP_STRING([--enable-coverage], [Enable code coverage tracking]))
AS_IF([test "x$enable_coverage" = "xyes"], [
    AC_DEFINE([COVERAGE], [], ["Link with gcov."])
    COVERAGE_FLAGS="--coverage"
])
AM_CONDITIONAL([COVERAGE], [test "x$enable_coverage" = "xyes"])

# Option to compile bf_runtime_grpc.
AC_ARG_ENABLE([grpc],
    AS_HELP_STRING([--enable-grpc], ["Enable grpc server for bf-runtime"]))

# Option to not compile debug-python-shell
AC_ARG_ENABLE([debug-python-shell],
    AS_HELP_STRING([--disable-debug-python-shell],
                   ["Don't compile debug-python shell, default=yes"]))
AM_CONDITIONAL([IS_DEBUG_PYTHON_SHELL_ENABLED], [test "x$enable_debug_python_shell" != "xno"])

# Option to not compile BF-RT
AC_ARG_ENABLE([bfrt],
    AS_HELP_STRING([--disable-bfrt],
                   ["Don't compile BF-RT"]))
AM_CONDITIONAL([IS_BFRT_ENABLED], [test "x$enable_bfrt" != "xno"])

AM_CONDITIONAL([IS_GRPC_ENABLED], [test "x$enable_grpc" = "xyes"])

# Checks for programs.
AC_PROG_CC_STDC
AC_PROG_CXX
LT_INIT

# enforce -std=c++11
AX_CXX_COMPILE_STDCXX_11([noext],[mandatory])

# Checks for header files.
AC_LANG_PUSH(C)
AC_LANG_PUSH(C++)

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T
AC_TYPE_UINT64_T
AC_C_INLINE
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_FUNC_STRERROR_R
AC_CHECK_FUNCS([clock_gettime])
AC_CHECK_FUNCS([floor])
AC_CHECK_FUNCS([gethostbyname])
AC_CHECK_FUNCS([getpagesize])
AC_CHECK_FUNCS([gettimeofday])
AC_CHECK_FUNCS([inet_ntoa])
AC_CHECK_FUNCS([memmove])
AC_CHECK_FUNCS([memset])
AC_CHECK_FUNCS([munmap])
AC_CHECK_FUNCS([pow])
AC_CHECK_FUNCS([regcomp])
AC_CHECK_FUNCS([setenv])
AC_CHECK_FUNCS([socket])
AC_CHECK_FUNCS([sqrt])
AC_CHECK_FUNCS([strchr])
AC_CHECK_FUNCS([strcspn])
AC_CHECK_FUNCS([strdup])
AC_CHECK_FUNCS([strerror])
AC_CHECK_FUNCS([strrchr])
AC_CHECK_FUNCS([strspn])
AC_CHECK_FUNCS([strstr])
AC_CHECK_FUNCS([strtol])
AC_CHECK_FUNCS([strtoul])
AC_CHECK_FUNCS([strtoull])
AC_CHECK_HEADER_STDBOOL
AC_CHECK_HEADERS([arpa/inet.h])
AC_CHECK_HEADERS([assert.h])
AC_CHECK_HEADERS([fcntl.h])
AC_CHECK_HEADERS([float.h])
AC_CHECK_HEADERS([inttypes.h])
AC_CHECK_HEADERS([limits.h])
AC_CHECK_HEADERS([malloc.h])
AC_CHECK_HEADERS([netdb.h])
AC_CHECK_HEADERS([netinet/in.h])
AC_CHECK_HEADERS([stdio.h])
AC_CHECK_HEADERS([stddef.h])
AC_CHECK_HEADERS([stdint.h])
AC_CHECK_HEADERS([stdlib.h])
AC_CHECK_HEADERS([string.h])
AC_CHECK_HEADERS([strings.h])
AC_CHECK_HEADERS([sys/file.h])
AC_CHECK_HEADERS([sys/ioctl.h])
AC_CHECK_HEADERS([sys/param.h])
AC_CHECK_HEADERS([sys/socket.h])
AC_CHECK_HEADERS([sys/stat.h])
AC_CHECK_HEADERS([sys/time.h])
AC_CHECK_HEADERS([sys/types.h])
AC_CHECK_HEADERS([syslog.h])
AC_CHECK_HEADERS([unistd.h])
AC_CHECK_TYPES([ptrdiff_t])
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_INT8_T
AC_TYPE_PID_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T

# Check for grpc support
AS_IF([test x"$enable_grpc" = "xyes"], [
PKG_CHECK_MODULES([GRPC], [grpc++ >= 1.17.0 grpc >= 7.0.0])
AC_SUBST([PROTOBUF_LIBS])
PKG_CHECK_MODULES([PROTOBUF], [protobuf >= 3.6.1])
AC_SUBST([GRPC_LIBS])

# Check for the following binaries under prefix first
AC_PATH_PROG([PROTOC], [protoc],
             [], ["$prefix/bin:$PATH"])

AS_IF([test x"$PROTOC" = x],
      [AC_MSG_ERROR([cannot find protoc])])

AC_PATH_PROG([GRPC_CPP_PLUGIN], [grpc_cpp_plugin],
             [], ["$prefix/bin:$PATH"])

AS_IF([test "x$GRPC_CPP_PLUGIN" = x],
      [AC_MSG_ERROR([cannot find grpc_cpp_plugin])])

AC_PATH_PROG([GRPC_PY_PLUGIN], [grpc_python_plugin],
             [], ["$prefix/bin:$PATH"])

AS_IF([test "x$GRPC_PY_PLUGIN" = x],
      [AC_MSG_WARN([cannot find grpc_python_plugin not found])])
])

# Check for installed libraries.
AC_CHECK_LIB([dl], [dlsym], [], [AC_MSG_ERROR([Missing dl])])
AC_CHECK_LIB([m], [sqrt], [], [AC_MSG_ERROR([Missing math library])])
AC_CHECK_LIB([pthread], [pthread_create], [LIBS+=-lpthread],
              [AC_MSG_ERROR([Missing pthread library])])
if test -n "$COVERAGE_FLAGS"; then
    AC_CHECK_LIB([gcov], [__gcov_init], [], [AC_MSG_ERROR([Missing gcov library])])
fi

AS_IF([test "x$enable_p4rt" = "xyes"], [
    AX_SAVE_FLAGS(PIcheck)
    CPPFLAGS="${CPPFLAGS} -I$bf_sysroot/$prefix/include"
    AC_CHECK_HEADER([PI/frontends/proto/device_mgr.h], [], [
        AC_MSG_ERROR(['--enable-p4rt' currently requires P4 Runtime dependencies to be installed using install_p4runtime_deps.sh])
    ])
    AX_RESTORE_FLAGS(PIcheck)
])

AC_LANG_POP(C++)

AM_CFLAGS=""
AC_ARG_WITH([kdrv],
            AS_HELP_STRING([--with-kdrv],
                           ["Building the kdrv kernel module, default=yes"]),
            [],
            [with_kdrv=yes])
AM_CONDITIONAL([IS_KDRV_WANTED], [test "x$with_kdrv" = "xyes"])

# Option to statically link all SDE libraries
AC_ARG_WITH([static-link-lib],
            AS_HELP_STRING([--with-static-link-lib], ["Statically link all SDE libraries"]))
AM_CONDITIONAL([IS_STATIC_LINKED_LIB],[test "x$with_static_link_lib" = "xyes"])

AC_ARG_ENABLE([linkdown-intr],
              AS_HELP_STRING([--enable-linkdown-intr], ["Enables link down interrupt, default=no"]))
AM_CONDITIONAL([IS_LINKDOWN_INTR_ENABLED], [test "x$enable_linkdown_intr" = "xyes"])

DRV_WRN_CFLAGS="$ASAN_FLAGS -Werror -Wall -Wextra -Wpedantic -Wshadow -DBF_SYS_LOG_FORMAT_CHECK -Wno-unused-parameter -Wno-missing-field-initializers -Wno-stringop-truncation"

AS_IF([test "x$(expr `gcc -dumpversion | cut -f1-2 -d.` \>= 10)" = "x1"],
     [DRV_WRN_CFLAGS+=" -Wno-error=enum-conversion -fcommon"])

DRV_UCLI_CFLAGS="-DUCLI_CONFIG_INCLUDE_ELS_LOOP=1 -DLLDLIB_CONFIG_INCLUDE_UCLI=1 -DPORT_MGR_CONFIG_INCLUDE_UCLI=1 -DDVM_CONFIG_INCLUDE_UCLI=1 -DPIPE_MGR_CONFIG_INCLUDE_UCLI=1 -DMC_MGR_CONFIG_INCLUDE_UCLI=1 -DTRAFFIC_MGR_CONFIG_INCLUDE_UCLI=1 -DDEVDIAG_CONFIG_INCLUDE_UCLI=1"
DRV_THRD_CFLAGS="-DPIPE_MGR_THREADS -DPIPE_MGR_INLINE_DR_SERVICE -DMC_MGR_THREADS -DMC_MGR_INLINE_DR_SERVICE -DTM_MT_SAFE"
DRV_MISC_CFLAGS="$GENERIC_FLAGS -DTARGET_IS_LLD -DINCLUDE_SERDES_PKG -DDEBUG_MODE=0 -DHARLYN_DEBUG_MODE=0"
DRV_CFLAGS="$DRV_WRN_CFLAGS $DRV_UCLI_CFLAGS $DRV_THRD_CFLAGS $DRV_MISC_CFLAGS"
#DRV_CXXFLAGS="-Wshadow -Wnon-virtual-dtor -Wold-style-cast -Wcast-align -Wunused -Woverloaded-virtual -Wconversion"
DRV_CXXFLAGS=""
AC_SUBST([BF_DRIVERS_CFLAGS], [$DRV_CFLAGS])
AC_SUBST([BF_DRIVERS_CXXFLAGS], ["$DRV_CFLAGS $DRV_CXXFLAGS"])

AC_SUBST([AM_CFLAGS], ["$AM_CFLAGS $COVERAGE_FLAGS"])
AC_SUBST([AM_CPPFLAGS], ["-I\$(top_srcdir)/include \
                          -I$bf_sysroot\$(prefix)/include/python3.8 \
                          -I$bf_sysroot\$(prefix)/include"])
AC_SUBST([AM_LDFLAGS], ["-rdynamic $COVERAGE_FLAGS -L$bf_sysroot\$(libdir)"])

AC_ARG_WITH([cint-binding],
            AS_HELP_STRING([--with-cint-binding],
                           ["Build the C interpreter bindings, default=no"]),
            [with_cint_binding=${withval}],
            [with_cint_binding=no])

AM_CONDITIONAL([IS_CINT_BINDING_WANTED], [test "x$with_cint_binding" = "xyes"])

AC_PATH_PROG([binding_generator], [gencint], [no], ["$PATH:$bf_sysroot/bin:$prefix/bin"])

AS_IF([test "x$binding_generator" == "xno"],[binding_generator="$prefix/bin/gencint" binding_dir="$prefix/lib"], [binding_dir="`$binding_generator --get-binding-dir`"])

AC_SUBST([BF_CINT_GEN], ["$binding_generator"])
AC_SUBST([BF_CINT_DIR], ["$binding_dir"])

#CHECK for doxygen
AC_CHECK_PROG([DOXYGEN], [doxygen], [doxygen])
#Always distribute these files
AC_CONFIG_FILES([doc/Doxyfile])
AC_CONFIG_FILES([doc/DoxygenLayout.xml])

AM_COND_IF([IS_BFRT_ENABLED],[AC_CONFIG_FILES([src/bf_rt/docs/Doxyfile])])
AM_COND_IF([IS_BFRT_ENABLED],[AC_CONFIG_FILES([src/bf_rt/docs/DoxygenLayout.xml])])
AM_COND_IF([IS_GRPC_ENABLED],[AC_CONFIG_FILES([src/bf_rt/bfruntime_grpc_client/python/docs/Doxyfile])])
AM_COND_IF([IS_GRPC_ENABLED],[AC_CONFIG_FILES([src/bf_rt/bfruntime_grpc_client/python/docs/DoxygenLayout.xml])])

#Set conditional
AM_CONDITIONAL([have_doxygen], [test -n "$DOXYGEN"])
#Always add doc Makefile
AC_CONFIG_FILES([doc/Makefile])
AM_COND_IF([IS_BFRT_ENABLED],[AC_CONFIG_FILES([src/bf_rt/docs/Makefile])])
AM_COND_IF([IS_GRPC_ENABLED],[AC_CONFIG_FILES([src/bf_rt/bfruntime_grpc_client/python/docs/Makefile])])

# Generate makefiles.
AC_CONFIG_FILES([Makefile
                 src/Makefile
                 include/Makefile
                 bf_switchd/Makefile
                 third-party/ptf_grpc/Makefile
		 src/lld/dpdk/Makefile
		 third-party/grpc/Makefile
		 third-party/Makefile
                 unit-test/Makefile])
AC_OUTPUT
