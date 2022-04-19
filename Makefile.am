ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS} -I m4

KDIR := /lib/modules/$(shell uname -r)/build
versiondir = $(datadir)
nobase_dist_version_DATA = VERSION

SUBDIRS = third-party third-party/grpc

if IS_GRPC_ENABLED
if IS_PTF_ENABLED
SUBDIRS += third-party/ptf_grpc
endif
endif

SUBDIRS += include src doc bf_switchd

if IS_UT_ENABLED
SUBDIRS += unit-test
endif

target_sysdir = $(datadir)/target_sys
dist_target_sys_DATA = \
zlog-cfg

PATCH_FILE = check.patch
#Add/remove fles from this list to get checkpatch tested
IGNORE_LIST = ':!*.patch' ':!.git*' ':!src/lld/old_files/*' \
		':!src/bf_pm/old_files/*' \
		':!src/port_mgr/old_files/*' \
		':!src/pipe_mgr/old_files/*' \
		':!bf_switchd/bf_switchd.c' \
		':!NAT_SDE_examples/*'

checkpatch:
	@echo "Checking latest commit patch"
	@git diff HEAD^ HEAD $(IGNORE_LIST) > $(PATCH_FILE)
	@$(KDIR)/scripts/checkpatch.pl --strict \
	--ignore GERRIT_CHANGE_ID,FILE_PATH_CHANGES,LINUX_VERSION_CODE,\
	NOT_UNIFIED_DIFF,GIT_COMMIT_ID,EMAIL_SUBJECT,PREFER_KERNEL_TYPES \
	--show-types --no-tree $(PATCH_FILE)|| \
        (echo -e "\nERROR: Please check the commit. It has style issues \n\n" \
	&& rm -f $(PATCH_FILE) && exit 1) && (rm -f $(PATCH_FILE))
