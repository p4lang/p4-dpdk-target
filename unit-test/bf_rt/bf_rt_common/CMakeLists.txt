cmake_minimum_required(VERSION 2.6)

include_directories(${CMAKE_SOURCE_DIR}/../src/bf_rt/bf_rt_common
                    ${CMAKE_SOURCE_DIR}/../include)


#Adding libary link directories
link_directories(${CMAKE_SOURCE_DIR}/../src/.libs)

set(CMAKE_EXE_LINKER_FLAGS_UT1 "-lgtest -ldriver -lcjson -ltarget_sys -Wl,--warn-unresolved-symbols -Wl,--no-export-dynamic")
add_executable(bf_rt_ut1_out test_main.cpp bf_rt_info_impl_ut1.cpp)
target_link_libraries(bf_rt_ut1_out ${CMAKE_EXE_LINKER_FLAGS_UT1})

set(CMAKE_EXE_LINKER_FLAGS_UT2 "-lgtest -Wl,--warn-unresolved-symbols -Wl,--no-export-dynamic")
add_executable(bf_rt_ut2_out test_main.cpp bf_rt_info_impl_ut2.cpp)
target_link_libraries(bf_rt_ut2_out ${CMAKE_EXE_LINKER_FLAGS_UT2})

set(FILES "bf_rt_ut1_out" "bf_rt_ut2_out") 

foreach(file ${FILES})
add_custom_command(
    TARGET ${file} POST_BUILD 
    COMMAND cp
    ${CMAKE_CURRENT_BINARY_DIR}/${file}
    ${CMAKE_INSTALL_PREFIX}/unit_test_result/${file})
endforeach()
