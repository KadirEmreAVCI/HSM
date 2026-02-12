cmake_minimum_required(VERSION 3.20)

add_executable(fsm_run_time_tests
  test_state_machine.cpp
)

target_link_libraries(fsm_run_time_tests
  PRIVATE
    fsm
    gtest
    gtest_main
    gmock
)
