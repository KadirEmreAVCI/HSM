# Toolchain for VxWorks 7 SR0640 with Clang
# Expected environment variables on the runner:
#   WIND_CC_SYSROOT    -> VxWorks SDK sysroot path
#   WIND_CLANG_TARGET  -> Clang target triple (example: x86_64-wrs-vxworks)

set(CMAKE_SYSTEM_NAME VxWorks)
set(CMAKE_SYSTEM_VERSION 7)

set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(DEFINED ENV{WIND_CC_SYSROOT} AND NOT "$ENV{WIND_CC_SYSROOT}" STREQUAL "")
    set(CMAKE_SYSROOT "$ENV{WIND_CC_SYSROOT}")
endif()

if(DEFINED ENV{WIND_CLANG_TARGET} AND NOT "$ENV{WIND_CLANG_TARGET}" STREQUAL "")
    set(CMAKE_C_FLAGS_INIT "--target=$ENV{WIND_CLANG_TARGET}")
    set(CMAKE_CXX_FLAGS_INIT "--target=$ENV{WIND_CLANG_TARGET}")
endif()

# Prevent CMake from linking/running test binaries at configure time in cross builds.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
