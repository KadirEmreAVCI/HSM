cmake_minimum_required(VERSION 3.20)

project(FSM VERSION 0.1.0 LANGUAGES CXX)

# -----------------------------
# Global C++ settings
# -----------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# -----------------------------
# Header-only library: fsm
# -----------------------------
add_library(fsm INTERFACE)

# Public include root:
#   #include <fsm/state_machine.h>  -> development/fsm/state_machine.h
target_include_directories(fsm
  INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/development
)

# Compiler-specific warnings (fixes MSVC /Wextra error)
target_compile_options(fsm
  INTERFACE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /permissive->
)

# -----------------------------
# Example executable
# -----------------------------
add_executable(fsm_main development/example_main.cpp)
target_link_libraries(fsm_main PRIVATE fsm)

# -----------------------------
# Testing + GoogleTest
# -----------------------------
include(CTest)
enable_testing()

set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)

add_subdirectory(extern/googletest)
add_subdirectory(test)
