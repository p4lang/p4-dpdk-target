# SPDX-FileCopyrightText: 2022 Anand Sunkad
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 2.6)

#Adding unit test folders
set(SUB_DIRS "tdi_common" "tdi_p4" "tdi_fixed")

if(CMAKE_ARGV1 STREQUAL "")
foreach(dir ${SUB_DIRS})
add_subdirectory(${dir})
endforeach()
else()
add_subdirectory(${CMAKE_ARGV1})
endif()

