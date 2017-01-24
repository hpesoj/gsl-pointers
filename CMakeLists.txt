cmake_minimum_required(VERSION 2.6)
project(observer_tests)
include_directories(${PROJECT_SOURCE_DIR}/tests ${PROJECT_SOURCE_DIR}/api)
add_executable(observer_tests tests/observer_tests.cpp)
