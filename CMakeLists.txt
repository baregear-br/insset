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

# Find LLVM for disassembly
find_package(LLVM 18.1 REQUIRED CONFIG)
message(STATUS "LLVM version: ${LLVM_PACKAGE_VERSION}")
message(STATUS "LLVM include dirs: ${LLVM_INCLUDE_DIRS}")
include_directories(${LLVM_INCLUDE_DIRS})
include_directories(/usr/lib/llvm-18/include)
add_definitions(${LLVM_DEFINITIONS})
llvm_map_components_to_libnames(LLVM_LIBS
    Support
    MC
    MCDisassembler
    Object
    BinaryFormat
    Target
    TargetParser
    X86Info
    X86Desc
    X86Disassembler
)

find_program(SPHINX_EXECUTABLE sphinx-build)
if(SPHINX_EXECUTABLE)
    add_custom_target(docs
            COMMAND ${SPHINX_EXECUTABLE} -b html ${CMAKE_SOURCE_DIR}/docs ${CMAKE_BINARY_DIR}/docs
            COMMENT "Building documentation with Sphinx"
            VERBATIM
    )
else()
    message(WARNING "Sphinx not found. Documentation target will not be available.")
endif()

include_directories(include)
add_subdirectory(src)