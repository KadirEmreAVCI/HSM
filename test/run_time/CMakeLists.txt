cmake_minimum_required(VERSION 3.20)

add_executable(hsm_run_time_tests
  test_state_machine.cpp
)

target_link_libraries(hsm_run_time_tests
  PRIVATE
    hsm
    gtest
    gtest_main
    gmock
)

target_include_directories(hsm_run_time_tests
  PRIVATE
    ${CMAKE_SOURCE_DIR}/example
)