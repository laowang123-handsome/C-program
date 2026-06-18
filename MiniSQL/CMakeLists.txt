cmake_minimum_required(VERSION 3.20)
project(MiniSQL LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include_directories(${CMAKE_SOURCE_DIR}/include)

add_library(minisql_core
    src/bplustree.cpp
    src/sql_parser.cpp
    src/storage_engine.cpp
    src/executor.cpp
    src/result_codec.cpp
    src/network.cpp
)

target_compile_options(minisql_core PRIVATE -Wall -Wextra -pedantic)

add_executable(minisql_server src/server_main.cpp)
target_link_libraries(minisql_server PRIVATE minisql_core)

add_executable(minisql_client src/client_main.cpp)
target_link_libraries(minisql_client PRIVATE minisql_core)

add_executable(minisql_tests src/tests_main.cpp)
target_link_libraries(minisql_tests PRIVATE minisql_core)

enable_testing()
add_test(NAME minisql_unit_tests COMMAND minisql_tests)
