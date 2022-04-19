cmake_minimum_required(VERSION 2.6)

project("unit-test")

#Adding include directories
include_directories(${CMAKE_INSTALL_PREFIX}/include 
                    ${CMAKE_SOURCE_DIR}/../src)

set(CMAKE_CXX_FLAGS "-fprofile-arcs -ftest-coverage -pthread -fpermissive -std=c++11 -O0 ${CMAKE_CXX_FLAGS}")

#Adding libary link directories
link_directories(${CMAKE_INSTALL_PREFIX}/lib
                 ${CMAKE_INSTALL_PREFIX}/lib64)

#Adding unit test folders
set(DIRS "bf_rt" "pipe_mgr") 

if(CMAKE_ARGV0 STREQUAL "")
foreach(dir ${DIRS})
add_subdirectory(${dir})
endforeach()
else()
add_subdirectory(${CMAKE_ARGV0})
endif()
