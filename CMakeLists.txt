cmake_minimum_required(VERSION 3.20)

project(HSM VERSION 0.1.0 LANGUAGES CXX)

# -----------------------------
# Global C++ settings
# -----------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# -----------------------------
# Header-only library: hsm
# -----------------------------
add_library(hsm INTERFACE)

# Public include root:
#   #include <hsm/state_machine.h>  -> development/hsm/state_machine.h
target_include_directories(hsm
  INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/development
)

# Compiler-specific warnings (fixes MSVC /Wextra error)
target_compile_options(hsm
  INTERFACE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /permissive->
)

# -----------------------------
# Example executable
# -----------------------------
add_executable(hsm_main example/example_main.cpp)
target_link_libraries(hsm_main PRIVATE hsm)

# -----------------------------
# Testing + GoogleTest
# -----------------------------
include(CTest)
enable_testing()

set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)

add_subdirectory(extern/googletest)
add_subdirectory(test)
