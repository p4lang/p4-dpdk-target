cmake_minimum_required(VERSION 3.5)
project(tdi_examples)
include(CMakePrintHelpers)
cmake_print_variables(CMAKE_INSTALL_PREFIX)
set(C_CXX_FLAGS "-Wall -Wextra -Werror -Wshadow -Wpedantic -Wno-missing-field-initializers -Wno-misleading-indentation -DBF_SYS_LOG_FORMAT_CHECK -L${CMAKE_INSTALL_PREFIX}/lib")
cmake_print_variables(C_CXX_FLAGS)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${C_CXX_FLAGS} -pthread")

set(CMAKE_SOURCE_DIR ../../)
cmake_print_variables(CMAKE_SOURCE_DIR)
# make sure the compiler can find following include files
include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/bf_switchd)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_INSTALL_PREFIX}/include)
get_property(dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
cmake_print_variables(${dirs})

#set(TESTDATADIR "")
link_directories(${CMAKE_SOURCE_DIR}/bf_switchd/.libs)
link_directories(${CMAKE_SOURCE_DIR}/src/.libs)
get_property(link_dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY LINK_DIRECTORIES)
cmake_print_variables(${link_dirs})

file( GLOB APP_SOURCES *.cpp *.c)
foreach( testsourcefile ${APP_SOURCES} )
    get_filename_component(testname ${testsourcefile} NAME_WE)
    # add executable called ${testname}_example. ex. pna_ipsec_notif_example
    add_executable( ${testname}_example ${testsourcefile} )
    # Link the current executable to the following libs
    target_link_libraries( ${testname}_example PUBLIC target_utils clish target_sys driver tdi bf_switchd_lib)
    install(TARGETS ${testname}_example DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
endforeach( testsourcefile ${APP_SOURCES} )
