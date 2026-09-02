;
; baregear - A programming language compiler
; Copyright (C) 2026 First Person
;
; This program is free software: you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation, either version 3 of the License, or
; (at your option) any later version.
;
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with this program.  If not, see <https://www.gnu.org/licenses/>.
;

global falloc
global frealloc
global ffree

; Usage (System V AMD64 ABI):
; RDI: Address (hint)
; RSI: Length
falloc:
    mov rax, 9          ; sys_mmap
    mov rdx, 3          ; 3rd arg: prot = PROT_READ (0x1) | PROT_WRITE (0x2)
    mov r10, 0x22       ; 4th arg: flags = MAP_PRIVATE (0x02) | MAP_ANONYMOUS (0x20)
    mov r8, -1          ; 5th arg: fd = -1
    mov r9, 0           ; 6th arg: offset = 0
    syscall             ; Note: This instruction will overwrite RCX and R11
    ret

; Usage (System V AMD64 ABI):
; RDI: Old address
; RSI: Old size
; RDX: New size
frealloc:
    mov rax, 25         ; sys_mremap
    mov r10, 1          ; MREMAP_MAYMOVE flags
    syscall             ; RAX will contain the NEW address of the memory
    ret

; Usage (System V AMD64 ABI):
; RDI: Address of allocated memory to free
; RSI: Length of the memory block
ffree:
    mov rax, 11         ; sys_munmap system call number
    syscall             ; Invoke the kernel (overwrites RCX and R11)
    ret