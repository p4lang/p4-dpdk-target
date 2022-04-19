cmake_minimum_required(VERSION 2.6)

include_directories(${CMAKE_SOURCE_DIR}/../third-party/gmock-global/include/gmock-global
                    ${CMAKE_SOURCE_DIR}/../src/lld/dpdk
                    ${CMAKE_SOURCE_DIR}/../include
                    ${CMAKE_SOURCE_DIR}/mock/include)

set(CMAKE_EXE_LINKER_FLAGS "-lgtest -lgmock -Wl,--warn-unresolved-symbols -Wl,--no-export-dynamic")
add_executable(lld_dpdk_port_out test_main.cpp lld_dpdk_port_ut1.cpp)
target_link_libraries(lld_dpdk_port_out ${CMAKE_EXE_LINKER_FLAGS})

set(FILES "lld_dpdk_port_out")

foreach(file ${FILES})
add_custom_command(
    TARGET ${file} POST_BUILD
    COMMAND cp
    ${CMAKE_CURRENT_BINARY_DIR}/${file}
    ${CMAKE_INSTALL_PREFIX}/unit_test_result/${file})
endforeach()
