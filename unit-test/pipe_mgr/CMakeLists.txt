cmake_minimum_required(VERSION 2.6)

#Adding unit test folders
set(SUB_DIRS "core") 

if(CMAKE_ARGV1 STREQUAL "")
foreach(dir ${SUB_DIRS})
add_subdirectory(${dir})
endforeach()
else()
add_subdirectory(${CMAKE_ARGV1})
endif()