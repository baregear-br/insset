cmake_minimum_required(VERSION 3.28)

if(EXISTS "${CMAKE_SOURCE_DIR}/vcpkg.json" AND NOT CMAKE_TOOLCHAIN_FILE)
    if(DEFINED ENV{VCPKG_ROOT})
        set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
    endif()
endif()

project(insset)
set(APP_VERSION 1.0)

find_package(PkgConfig REQUIRED)
find_library(LIEF_LIBRARY NAMES LIEF PATHS /usr/lib NO_DEFAULT_PATH)
find_path(LIEF_INCLUDE_DIR NAMES LIEF/LIEF.hpp PATHS /usr/include NO_DEFAULT_PATH)
if(NOT LIEF_LIBRARY OR NOT LIEF_INCLUDE_DIR)
    message(FATAL_ERROR "LIEF library not found")
endif()
pkg_check_modules(CAPSTONE REQUIRED capstone)
include_directories(${CAPSTONE_INCLUDE_DIRS})
include_directories(include)

add_subdirectory(src)