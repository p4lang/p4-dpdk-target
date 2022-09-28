cmake_minimum_required(VERSION 2.6)

include_directories(${CMAKE_SOURCE_DIR}/../third-party/gmock-global/include/gmock-global
                    ${CMAKE_SOURCE_DIR}/../src/pipe_mgr/shared/features
                    ${CMAKE_SOURCE_DIR}/../src/lld/dpdk/dpdk_src/lib/pipeline
                    ${CMAKE_SOURCE_DIR}/../include
                    ${CMAKE_SOURCE_DIR}/mock/include)

set(CMAKE_EXE_LINKER_FLAGS "-lgtest -lgmock -ldriver -lcjson -lbf_switchd_lib -ltarget_utils -ltarget_sys  -lclish  -Wl,--warn-unresolved-symbols -Wl,--no-export-dynamic")
add_executable(dal_value_lookup_out test_main.cpp dal_value_lookup_ut1.cpp)
target_link_libraries(dal_value_lookup_out ${CMAKE_EXE_LINKER_FLAGS})

set(FILES "dal_value_lookup_out")

foreach(file ${FILES})
add_custom_command(
    TARGET ${file} POST_BUILD
    COMMAND cp
    ${CMAKE_CURRENT_BINARY_DIR}/${file}
    ${CMAKE_INSTALL_PREFIX}/unit_test_result/${file})
endforeach()
