cmake_minimum_required(VERSION 2.6)

add_subdirectory("dal")

include_directories(${CMAKE_SOURCE_DIR}/../third-party/gmock-global/include/gmock-global
                    ${CMAKE_SOURCE_DIR}/../src/pipe_mgr/core
                    ${CMAKE_SOURCE_DIR}/../include
                    ${CMAKE_SOURCE_DIR}/mock/include)

set(CMAKE_EXE_LINKER_FLAGS "-ltarget_sys -lcjson -lgtest -lgmock -Wl,--warn-unresolved-symbols -Wl,--no-export-dynamic")
add_executable(ctx_json_out test_main.cpp pipe_mgr_ctx_json_ut1.cpp)
add_executable(ctx_json_out_2 test_main.cpp pipe_mgr_ctx_json_ut2.cpp)
target_link_libraries(ctx_json_out ${CMAKE_EXE_LINKER_FLAGS})
target_link_libraries(ctx_json_out_2 ${CMAKE_EXE_LINKER_FLAGS})

set(FILES "ctx_json_out ctx_json_out_2") 

foreach(file ${FILES})
add_custom_command(
    TARGET ${file} POST_BUILD 
    COMMAND cp
    ${CMAKE_CURRENT_BINARY_DIR}/${file}
    ${CMAKE_INSTALL_PREFIX}/unit_test_result/${file})
endforeach()
