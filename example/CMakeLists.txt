set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin/example)

set(MY_LIBS
    boost_unit_test_framework
    boost_test_exec_monitor
    bbt_core
    bbt_network
)

add_executable(test_client client.cc)
target_link_libraries(test_client ${MY_LIBS})

add_executable(test_server server.cc)
target_link_libraries(test_server ${MY_LIBS})

add_executable(test_evthread evthread.cc)
target_link_libraries(test_evthread ${MY_LIBS})

add_executable(echo_server echo_server.cc)
target_link_libraries(echo_server ${MY_LIBS})

add_executable(echo_client echo_client.cc)
target_link_libraries(echo_client ${MY_LIBS})