# Contributing

## Build Instructions

```bash
cmake -B build
cd build
make -j8
make docs # Build The Documentation Of insset Library Using Sphinx
```
Note: Rich Text Documentation Is Located On `build/docs`

## Source Tree

```
insset/
├── CMakeLists.txt                    # Root CMake build configuration
├── README.md                         # Project overview and documentation
├── CONTRIBUTING.md                   # This file - contribution guidelines
├── include/                          # Public header files
│   ├── analyzer.h                   # Binary analysis framework and fault detection
│   ├── definations.h                 # Common definitions and error handling
│   ├── dynvar.h                     # Dynamic variable and vector data structures
│   ├── runtime.h                    # Runtime memory management functions
│   ├── threading.h                  # Threading and concurrency utilities
│   └── insset/                      # Architecture-specific instruction set definitions
│       ├── cominsset.h              # Common instruction set operators and enums
│       ├── x86.h                    # x86 (32-bit) register structures and setup
│       └── x86_64.h                 # x86-64 (64-bit) register structures and setup
└── src/                             # Implementation files
    ├── CMakeLists.txt               # Source-level CMake configuration
    ├── analyzer.cpp                 # Binary analysis using LLVM and LIEF
    ├── cominsset.c                  # Common instruction implementations (ADD, SUB, MUL, etc.)
    ├── dynvar.c                     # Dynamic variable and vector implementation
    ├── runtime.asm                  # Low-level memory management (mmap/mremap/munmap)
    └── threading.c                  # POSIX threading wrapper implementation
```

## File Descriptions

### Core Components

**`include/dynvar.h` & `src/dynvar.c`**
- Dynamic variable system for handling variable-length data
- Vector data structure for dynamic arrays
- Memory management for string and data operations
- Key functions: `vectorInit`, `vectorAppend`, `vectorDelete`, `setValue`, `getValue`

**`include/runtime.h` & `src/runtime.asm`**
- Low-level memory management using system calls
- Functions: `falloc` (mmap), `frealloc` (mremap), `ffree` (munmap)
- Direct system call interface for memory operations
- Platform-specific for Linux x86-64

**`include/threading.h` & `src/threading.c`**
- Thread management wrapper around POSIX threads
- Thread creation, joining, detachment, and cleanup
- Thread lifecycle tracking and state management
- Mutex-protected thread registry for safe concurrent access

### Instruction Set Simulation

**`include/insset/cominsset.h` & `src/cominsset.c`**
- Common instruction set implementations for multiple architectures
- Arithmetic operations: ADD, SUB, MUL, DIV, INC, DEC
- Logical operations: AND, OR, XOR, NOT, SHIFT
- Control flow: CMP, TEST, JMP, conditional jumps
- CPU flag management (ZF, SF, CF, OF, PF)
- Architecture-agnostic processor interface

**`include/insset/x86.h`**
- x86 (32-bit) register structure definitions
- Complete register set: GPRs, segment registers, control registers, debug registers
- FPU, MMX, XMM register definitions
- Register offset calculation and setup function
- Support for 8-bit, 16-bit, and 32-bit register access

**`include/insset/x86_64.h`**
- x86-64 (64-bit) register structure definitions
- Extended register set: RAX-R15, additional control/debug registers
- AVX extensions: YMM and ZMM registers
- FS/GS base addresses and MSR support
- Comprehensive register mapping for 64-bit, 32-bit, 16-bit, and 8-bit access

### Binary Analysis

**`include/analyzer.h` & `src/analyzer.cpp`**
- Binary analysis framework using LLVM and LIEF libraries
- ELF binary parsing and symbol resolution
- Instruction disassembly using LLVM MC layer
- Fault detection and behavioral analysis
- Real-time operation monitoring with threading
- AST-based instruction representation and evaluation

### Build System

**`CMakeLists.txt` (root)**
- Top-level project configuration
- Dependency management (LLVM, LIEF, pthread)
- Build options and compiler flags
- Subdirectory inclusion for src/

**`src/CMakeLists.txt`**
- Source file compilation rules
- Assembly file handling for runtime.asm
- Library linking and output configuration

## Architecture Support

The library supports multiple LLVM architectures:
- x86, x86_64 (primary focus with full register emulation)
- ARM, AArch64 (enum definitions, partial support)
- MIPS, PowerPC, RISC-V, and others (enum definitions only)

## Development Notes

- All source files are licensed under GPL v3
- The project uses C and C++ mixed compilation
- Assembly code is x86-64 Linux specific
- Thread safety is ensured through mutex protection
- Memory management uses direct system calls for efficiency