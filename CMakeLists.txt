cmake_minimum_required(VERSION 3.28)

if(EXISTS "${CMAKE_SOURCE_DIR}/vcpkg.json" AND NOT CMAKE_TOOLCHAIN_FILE)
    if(DEFINED ENV{VCPKG_ROOT})
        set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
    endif()
endif()

project(insset LANGUAGES C CXX)
set(APP_VERSION 1.0)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

find_package(LLVM REQUIRED CONFIG)
find_package(PkgConfig REQUIRED)
pkg_check_modules(CAPSTONE REQUIRED capstone)
include_directories(${CAPSTONE_INCLUDE_DIRS})
include_directories(${LLVM_INCLUDE_DIRS})
include_directories(include)

add_subdirectory(src)