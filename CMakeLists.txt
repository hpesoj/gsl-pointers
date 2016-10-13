cmake_minimum_required(VERSION 2.6)
project(view_tests)
include_directories(${PROJECT_SOURCE_DIR}/tests ${PROJECT_SOURCE_DIR}/api)
add_executable(view_tests tests/view_tests.cpp)
