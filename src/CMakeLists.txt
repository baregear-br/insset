# Find NASM assembler
find_program(NASM_EXECUTABLE nasm)

if(NOT NASM_EXECUTABLE)
    message(FATAL_ERROR "NASM assembler not found. Please install NASM.")
endif()

# Detect target architecture (32-bit vs 64-bit)
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(NASM_FORMAT elf64)
    message(STATUS "Building runtime.asm for 64-bit (elf64)")
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(NASM_FORMAT elf32)
    message(STATUS "Building runtime.asm for 32-bit (elf32)")
else()
    message(FATAL_ERROR "Unsupported architecture: pointer size is ${CMAKE_SIZEOF_VOID_P}")
endif()

# Assemble runtime.asm with maximum optimization
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/runtime.o
    COMMAND ${NASM_EXECUTABLE}
        -f ${NASM_FORMAT}
        ${CMAKE_CURRENT_SOURCE_DIR}/runtime.asm
        -o ${CMAKE_CURRENT_BINARY_DIR}/runtime.o
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/runtime.asm
    COMMENT "Assembling runtime.asm with NASM (${NASM_FORMAT})"
)

# Create shared library from object file and C source
add_library(insset SHARED ${CMAKE_CURRENT_BINARY_DIR}/runtime.o dynvar.c cominsset.c analyzer.c)

# Link with LIEF and Capstone
target_link_libraries(insset PRIVATE
    ${LIEF_LIBRARY}
    ${CAPSTONE_LIBRARIES}
)
target_include_directories(insset PRIVATE ${LIEF_INCLUDE_DIR})

# Set optimization flags for maximum speed and lowest latency
target_compile_options(insset PRIVATE
    -O3                    # Maximum optimization
    -march=native          # Optimize for current CPU
    -mtune=native          # Tune for current CPU
    -flto                  # Link-time optimization
    -ffast-math            # Fast math operations
    -funroll-loops         # Loop unrolling
)

# Enable link-time optimization
set_target_properties(insset PROPERTIES
    INTERPROCEDURAL_OPTIMIZATION TRUE
)
