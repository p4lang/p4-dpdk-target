#!/bin/bash
##
## Copyright(c) 2021 Intel Corporation.
##
## Licensed under the Apache License, Version 2.0 (the "License");
## you may not use this file except in compliance with the License.
## You may obtain a copy of the License at
##
## http://www.apache.org/licenses/LICENSE-2.0
##
## Unless required by applicable law or agreed to in writing, software
## distributed under the License is distributed on an "AS IS" BASIS,
## WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
## See the License for the specific language governing permissions and
## limitations under the License.
##

apply_patch()
{
	local PATCH_FILES=(
		0001-fix-out-of-bounds-check-label.patch
	)

	DPDK_PATH=${PWD}
	DPDK_SRC_PATH="${DPDK_PATH}"/dpdk_src
	DPDK_PATCH_PATH="${DPDK_PATH}"/patch
	DPDK_SRC_GIT_FILE="${DPDK_SRC_PATH}"/.git

	# Skip patching if branch is already compiled.
	if [ -d "${DPDK_SRC_PATH}/build" ]; then
		exit 1;
	fi
	# Need to check .git in dpdk_src. There are some cases where we remove .git after patch
	# apply and start the compilaltion.
	if [ -e ${DPDK_SRC_GIT_FILE} ]; then
		# Let's clean all the changes
		(cd "${DPDK_SRC_PATH}"; git checkout *)

		# Validate and apply the patch
		for i in "${PATCH_FILES[@]}"; do
			if [ -e "${DPDK_PATCH_PATH}/${i}" ]; then
				(cd "${DPDK_SRC_PATH}"; git apply "${DPDK_PATCH_PATH}/${i}")
			fi
		done
	else
		# This is special case where we don't have .git dir. For example sandbox
		# doesn't copy git releated files. As a workaround we have to apply the
		# patch using <patch -p1>

		# Let's clean all the changes
		for i in "${PATCH_FILES[@]}"; do
			if [ -e "${DPDK_PATCH_PATH}/${i}" ]; then
				(cd "${DPDK_SRC_PATH}"; patch --no-backup-if-mismatch -p1 -Rfs -r - < "${DPDK_PATCH_PATH}/${i}")
			fi
		done

		# Validate and apply the patch
		for i in "${PATCH_FILES[@]}"; do
			if [ -e "${DPDK_PATCH_PATH}/${i}" ]; then
				(cd "${DPDK_SRC_PATH}"; patch -p1 < "${DPDK_PATCH_PATH}/${i}")
			fi
		done
	fi
}

apply_patch
