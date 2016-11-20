cmake_minimum_required(VERSION 2.6)
project(indirect_tests)
include_directories(${PROJECT_SOURCE_DIR}/tests ${PROJECT_SOURCE_DIR}/api)
add_executable(indirect_tests tests/indirect_tests.cpp)
