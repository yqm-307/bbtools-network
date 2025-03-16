set(Wall_Flag "-Wall -Wno-sign-compare -Wno-format -Wno-reorder -Wno-unknown-pragmas")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 ${Wall_Flag} -g")

enable_testing()
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin/unit_test)

set(MY_LIBS
    boost_unit_test_framework
    boost_test_exec_monitor
    bbt_core
    bbt_network
)

add_executable(Test_Client client.cc)
target_link_libraries(Test_Client ${MY_LIBS})

add_executable(Test_Server server.cc)
target_link_libraries(Test_Server ${MY_LIBS})

add_executable(test_evthread evthread.cc)
target_link_libraries(test_evthread ${MY_LIBS})

# add_executable(echo_server echo_server.cc)
# target_link_libraries(echo_server ${MY_LIBS})

# add_executable(echo_client echo_client.cc)
# target_link_libraries(echo_client ${MY_LIBS})