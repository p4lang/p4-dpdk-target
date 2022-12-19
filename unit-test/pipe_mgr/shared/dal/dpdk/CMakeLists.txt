cmake_minimum_required(VERSION 2.6)

include_directories(${CMAKE_SOURCE_DIR}/../third-party/gmock-global/include/gmock-global
                    ${CMAKE_SOURCE_DIR}/../src/pipe_mgr/shared/dal/dpdk
		    ${CMAKE_SOURCE_DIR}/../src/lld/dpdk/dpdk_src/lib/pipeline
                    ${CMAKE_SOURCE_DIR}/../include
                    ${CMAKE_SOURCE_DIR}/mock/include
		    ${CMAKE_SOURCE_DIR}/../src/lld/dpdk)

set(CMAKE_EXE_LINKER_FLAGS "-lgtest -lgmock -Wl,--warn-unresolved-symbols -Wl,--no-export-dynamic")
add_executable(dal_dpdk_mirror_out test_main.cpp dal_dpdk_mirror_ut1.cpp)
add_executable(dal_mat_ctx_out test_main.cpp dal_mat_store_entries_ut1.cpp)
add_executable(dal_dpdk_counters_out test_main.cpp dal_dpdk_counters_ut.cpp)
add_executable(dal_dpdk_registers_out test_main.cpp dal_dpdk_registers_ut.cpp)

target_link_libraries(dal_dpdk_mirror_out ${CMAKE_EXE_LINKER_FLAGS})
target_link_libraries(dal_mat_ctx_out ${CMAKE_EXE_LINKER_FLAGS})
target_link_libraries(dal_dpdk_counters_out ${CMAKE_EXE_LINKER_FLAGS})
target_link_libraries(dal_dpdk_registers_out ${CMAKE_EXE_LINKER_FLAGS})

set(FILES "dal_dpdk_mirror_out" "dal_mat_ctx_out" "dal_dpdk_counters_out" "dal_dpdk_registers_out" )

foreach(file ${FILES})
add_custom_command(
    TARGET ${file} POST_BUILD
    COMMAND cp
    ${CMAKE_CURRENT_BINARY_DIR}/${file}
    ${CMAKE_INSTALL_PREFIX}/unit_test_result/${file})
endforeach()
