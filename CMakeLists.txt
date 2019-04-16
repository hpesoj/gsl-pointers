cmake_minimum_required(VERSION 3.14)

project(gsl-pointers)

include(CTest)

add_executable(tests
  tests/test.cpp
  tests/test_optional_ref.cpp
  tests/test_retained.cpp
)


target_compile_features(tests
  PUBLIC
  cxx_std_17
)

target_include_directories(tests
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/api
  ${CMAKE_CURRENT_SOURCE_DIR}/tests
)

add_test(
  NAME "tests"
  COMMAND "tests"
)
