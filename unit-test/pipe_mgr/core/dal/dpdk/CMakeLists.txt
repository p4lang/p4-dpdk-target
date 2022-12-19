cmake_minimum_required(VERSION 2.6)

include_directories(${CMAKE_SOURCE_DIR}/../third-party/gmock-global/include/gmock-global
                    ${CMAKE_SOURCE_DIR}/../src/pipe_mgr/core/dal/dpdk
                    ${CMAKE_SOURCE_DIR}/../include
                    ${CMAKE_SOURCE_DIR}/mock/include)

set(CMAKE_EXE_LINKER_FLAGS "-lgtest -lgmock -ldriver -lcjson -lbf_switchd_lib -ltarget_utils -ltarget_sys  -lclish  -Wl,--warn-unresolved-symbols -Wl,--no-export-dynamic")
add_executable(pipe_mgr_ctx_dpdk_json_out test_main.cpp pipe_mgr_ctx_dpdk_json_ut.cpp)
target_link_libraries(pipe_mgr_ctx_dpdk_json_out ${CMAKE_EXE_LINKER_FLAGS})

set(FILES "pipe_mgr_ctx_dpdk_json_out")

foreach(file ${FILES})
add_custom_command(
    TARGET ${file} POST_BUILD
    COMMAND cp
    ${CMAKE_CURRENT_BINARY_DIR}/${file}
    ${CMAKE_INSTALL_PREFIX}/unit_test_result/${file})
endforeach()
