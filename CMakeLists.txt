cmake_minimum_required(VERSION 3.1)

project(bbtools-network)

# set(CMAKE_CXX_FALGS "${CMAKE_CXX_FLAGS} -std=c++17 -g -O2")
set(CMAKE_CXX_FALGS "${CMAKE_CXX_FLAGS} -std=c++17 -g")

include_directories(bbt)

set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib) 
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
include_directories(
    ${PROJECT_SOURCE_DIR}
)

file(GLOB_RECURSE SRCS 
    ${PROJECT_SOURCE_DIR}/bbt/*.cc
)

add_library(bbt_network SHARED ${SRCS})

target_link_libraries(bbt_network
    event_core
    event_pthreads
    pthread
)

add_subdirectory(unit_test)
enable_testing()