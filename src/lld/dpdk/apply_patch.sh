#!/bin/bash

# Copyright (C) 2021 Intel Corporation.
# SPDX-FileCopyrightText: 2021 James Choi
#
# SPDX-License-Identifier: Apache-2.0

apply_patch()
{
	local
  PATCH_FILES=(0001-Copy-required-header-file-to-install-include-path.patch)

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
		(cd "${DPDK_SRC_PATH}"; git clean -xfd; git checkout *)

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
