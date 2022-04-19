if (NOT PYTHON_EXECUTABLE)
  set(PYTHON_EXECUTABLE python)
endif()

execute_process(
  COMMAND ${PYTHON_EXECUTABLE} -c "if True:
    from distutils import sysconfig as sc
    print(sc.get_python_lib(prefix='', standard_lib=True, plat_specific=True))"
  OUTPUT_VARIABLE PYTHON_SITE
  OUTPUT_STRIP_TRAILING_WHITESPACE)
set(PYTHON_SITE "${PYTHON_SITE}/site-packages")

set(abs_srcdir ${CMAKE_CURRENT_SOURCE_DIR})
set(prefix ${CMAKE_INSTALL_PREFIX})
set(pythondir ${CMAKE_INSTALL_PREFIX}/${PYTHON_SITE})
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/run_ptf_tests.py.in ${CMAKE_CURRENT_BINARY_DIR}/run_ptf_tests.py)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/run_ptf_tests.py DESTINATION ${PYTHON_SITE}/p4testutils)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/bf_switchd_dev_status.py DESTINATION ${PYTHON_SITE}/p4testutils)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/ptf_interfaces.py DESTINATION ${PYTHON_SITE}/p4testutils)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/erspan3.py DESTINATION ${PYTHON_SITE}/p4testutils)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/pd_base_tests.py DESTINATION ${PYTHON_SITE}/p4testutils)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/bfruntime_base_tests.py DESTINATION ${PYTHON_SITE}/p4testutils)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/bfruntime_client_base_tests.py DESTINATION ${PYTHON_SITE}/p4testutils)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/port_mapping.py DESTINATION ${PYTHON_SITE}/p4testutils)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/ptf_port.py DESTINATION ${PYTHON_SITE}/p4testutils)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/bfd_utils.py DESTINATION ${PYTHON_SITE}/p4testutils)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/traffic_utils.py DESTINATION ${PYTHON_SITE}/p4testutils)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/traffic_streams.py DESTINATION ${PYTHON_SITE}/p4testutils)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/traffic_protocols.py DESTINATION ${PYTHON_SITE}/p4testutils)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/tg-wrapper/TrafficGen.py DESTINATION ${PYTHON_SITE}/p4testutils)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/p4runtime_base_tests.py DESTINATION ${PYTHON_SITE}/p4testutils)

install(PROGRAMS ${CMAKE_CURRENT_SOURCE_DIR}/port_mapping_clean DESTINATION bin)
install(PROGRAMS ${CMAKE_CURRENT_SOURCE_DIR}/port_mapping_setup DESTINATION bin)
install(PROGRAMS ${CMAKE_CURRENT_SOURCE_DIR}/port_ifup DESTINATION bin)
install(PROGRAMS ${CMAKE_CURRENT_SOURCE_DIR}/port_ifdown DESTINATION bin)
install(PROGRAMS ${CMAKE_CURRENT_SOURCE_DIR}/veth_setup.sh DESTINATION bin)
install(PROGRAMS ${CMAKE_CURRENT_SOURCE_DIR}/veth_teardown.sh DESTINATION bin)
install(PROGRAMS ${CMAKE_CURRENT_SOURCE_DIR}/dma_setup.sh DESTINATION bin)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/mavericks_port_mapping.json DESTINATION share/p4testutils)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/montara_port_mapping.json DESTINATION share/p4testutils)
