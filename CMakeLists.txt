cmake_minimum_required(VERSION 2.6)
project(gsl_unit_tests)
include_directories(${PROJECT_SOURCE_DIR}/tests ${PROJECT_SOURCE_DIR}/api)
add_executable(unit_tests tests/unit_tests.cpp)
