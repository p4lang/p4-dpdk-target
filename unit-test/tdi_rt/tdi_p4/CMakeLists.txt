cmake_minimum_required(VERSION 2.6)

include_directories(${CMAKE_SOURCE_DIR}/../src/tdi_rt/
                    ${CMAKE_SOURCE_DIR}/../src/bf_rt/bf_rt_common
                    ${CMAKE_SOURCE_DIR}/../src/bf_rt/
                    ${CMAKE_SOURCE_DIR}/../src/tdi_rt/tdi_common
                    ${CMAKE_SOURCE_DIR}/../src/tdi_rt/tdi_p4
                    ${CMAKE_SOURCE_DIR}/../src/tdi_rt/tdi_port/dpdk
                    ${CMAKE_SOURCE_DIR}/../third-party/gmock-global/include/gmock-global
                    ${CMAKE_SOURCE_DIR}/../unit-test/tdi_rt/tdi_common
                    ${CMAKE_SOURCE_DIR}/../include)

#Adding libary link directories
link_directories(${CMAKE_SOURCE_DIR}/../src/.libs)

set(CMAKE_EXE_LINKER_FLAGS_UT1 "-lgtest -lgmock -ldriver -lcjson -ltarget_sys -ltdi -ltdi_pna -lclish -lcjson -ltdi_json_parser -Wl,--warn-unresolved-symbols -Wl,--no-export-dynamic")

add_executable(tdi_reg_ut_out test_main.cpp tdi_reg_ut.cpp)
target_link_libraries(tdi_reg_ut_out ${CMAKE_EXE_LINKER_FLAGS_UT1})

set(FILES "tdi_reg_ut_out")

target_compile_options(tdi_reg_ut_out PRIVATE
  "-DJSONDIR=\"${CMAKE_CURRENT_SOURCE_DIR}/tdi_json_files\""
)

foreach(file ${FILES})
add_custom_command(
    TARGET ${file} POST_BUILD
    COMMAND cp
    ${CMAKE_CURRENT_BINARY_DIR}/${file}
    ${CMAKE_INSTALL_PREFIX}/unit_test_result/${file})
endforeach()
