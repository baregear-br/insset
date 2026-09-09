API Usage
=========

Overview of the ``insset`` shared library (``libinsset``).
Include headers from ``include/`` and link with ``insset``, ``pthread``,
``LIEF`` and LLVM 18.1.

.. code-block:: c

   #include <dynvar.h>
   #include <runtime.h>
   #include <threading.h>
   #include <analyzer.h>
   #include <definations.h>
   #include <insset/cominsset.h>

   // cc main.c -linsset -lpthread -lLIEF

All ``cominsset.h``, ``dynvar.h``, ``threading.h`` and ``runtime.h``
functions are ``extern "C"`` safe and usable from C and C++.

dynvar.h - Dynamic Variables And Vectors
----------------------------------------

Core types. All containers use the ``falloc`` / ``frealloc`` / ``ffree``
allocator from ``runtime.h``.

.. code-block:: c

   #define MAX_STACK_SIZE (64 * 1024)
   typedef char lgr[MAX_STACK_SIZE];

   #define VECTOR_FORMULA(var, idx) \
     ((void *)((char *)(var)->address + ((idx) * (var)->sizePerBlks)))

   typedef struct { uintptr_t address; unsigned int length; } dynvar;

   typedef struct {
     uintptr_t address;
     unsigned int sizePerBlks;
     unsigned int count;
   } vector;

   typedef enum { DYNVAR_SUCCESS, ILVAR, BFROVRFLW } DYNVAR_CODE;

   extern vector emvec;

``emvec`` is a global scratch vector used by the analyzer
(e.g. passed as ``eop`` / ``eoplen`` to ``add``).

vectorInit
~~~~~~~~~~

.. code-block:: c

   void vectorInit(vector *var, unsigned int length);

Initialize ``var`` with element size ``length``. Frees previous
content if ``var->address`` is set. Call once before use.

.. code-block:: c

   vector v;
   vectorInit(&v, sizeof(int));

vectorAppend
~~~~~~~~~~~~

.. code-block:: c

   DYNVAR_CODE vectorAppend(vector *var, lgr source);

Copy ``var->sizePerBlks`` bytes from ``source`` to the end of ``var``.
Returns ``DYNVAR_SUCCESS``, or ``ILVAR`` if not initialized with
``vectorInit``.

.. code-block:: c

   lgr buf;
   memcpy(buf, &value, sizeof(value));
   vectorAppend(&v, buf);

vectorGetValue
~~~~~~~~~~~~~~

.. code-block:: c

   long vectorGetValue(vector *var, int index);

Return pointer to element ``index`` cast to ``long``.
Returns ``-BFROVRFLW`` on out-of-bounds access.

.. code-block:: c

   AST *node = (AST *)vectorGetValue(&v, 0);

vectorFind
~~~~~~~~~~

.. code-block:: c

   int vectorFind(vector *var, long value);

Linear search. Returns index or ``-1`` if not found.

vectorDelete / vectorDeleteAll
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: c

   DYNVAR_CODE vectorDelete(vector *var, int index);
   void vectorDeleteAll(vector *var);

``vectorDelete`` removes one element and shifts the tail down.
Returns ``BFROVRFLW`` on bad index. ``vectorDeleteAll`` frees the
whole storage with ``ffree`` and resets ``address`` / ``count``.

setValue / getValue
~~~~~~~~~~~~~~~~~~~

.. code-block:: c

   void setValue(dynvar *var, lgr value);
   void getValue(dynvar var, lgr out);

NUL-terminated string helpers. ``setValue`` allocates / reallocates
with ``falloc`` / ``frealloc`` and truncates to ``MAX_STACK_SIZE - 1``.
``getValue`` zero-fills ``out`` (must be ``lgr``) and copies
``var.length`` bytes.

.. code-block:: c

   dynvar name = {0, 0};
   setValue(&name, "my_function");
   lgr out;
   getValue(name, out);

runtime.h - Low Level Allocator
-------------------------------

Implemented in ``src/runtime.asm`` with raw ``mmap`` / ``mremap`` /
``munmap`` syscalls (System V AMD64 ABI). No ``malloc`` involved.

.. code-block:: c

   void *falloc(void *address, size_t length);
   void *frealloc(void *old_address, size_t old_size, size_t new_size);
   int ffree(void *address, size_t length);

- ``falloc(address, length)``: ``mmap`` anonymous private memory.
  Pass ``NULL`` as hint for a fresh block.
- ``frealloc(old_address, old_size, new_size)``: ``mremap`` with
  ``MREMAP_MAYMOVE``. Returns new address.
- ``ffree(address, length)``: ``munmap``. Returns kernel status.

.. code-block:: c

   #include <runtime.h>

   void *p = falloc(NULL, 64);
   p = frealloc(p, 64, 128);
   ffree(p, 128);

threading.h - Thread Pool
-------------------------

Light wrapper over ``pthread`` tracking handles in a ``vector``.

.. code-block:: c

   void threadingInit();
   int threadNew(void callback(void *), ...);
   int threadJoin(int threadId);
   int threadDetach(int threadId);
   bool threadIsRunning(int threadId);
   void threadCleanup();

Call ``threadingInit()`` once before ``threadNew``. ``threadNew``
takes a ``void (*)(void *)`` callback plus one optional ``void *``
argument, returns a small integer thread id or ``-1`` on failure.

.. code-block:: c

   #include <threading.h>

   void work(void *arg) { /* ... */ }

   threadingInit();
   int id = threadNew(work, myArg);
   threadJoin(id);
   threadCleanup();

- ``threadJoin(id)``: join running thread, returns ``0`` or ``-1``.
- ``threadDetach(id)``: detach running thread, returns ``0`` or ``-1``.
- ``threadIsRunning(id)``: poll state.
- ``threadCleanup()``: join all remaining threads and free the list.

definations.h - Error Reporting
-------------------------------

.. code-block:: c

   void bugDetected(char *message);

Report a fatal analyzer / simulator fault (unknown arch, LLVM init
failure, invalid register). The caller keeps ownership of ``message``.

.. code-block:: c

   #include <definations.h>

   char *msg = strdup("Disassembler Is Failed To Initialize.");
   bugDetected(msg);
   free(msg);

insset/cominsset.h - Instruction Simulation
-------------------------------------------

Simulate common x86 / x86_64 ALU operations against the virtual
register file in ``registers``. Must call ``cinit`` first.

.. code-block:: c

   extern uintptr_t registers;
   extern int reglen;
   void cinit(LLVMArch arch);

``cinit`` allocates ``x86_64_Registers`` or ``x86_Registers`` with
``falloc`` and is idempotent. ``registers`` holds the base address,
``reglen`` the struct size.

.. code-block:: c

   #include <insset/cominsset.h>

   cinit(x86_64);
   x86_64_Registers *regs = (x86_64_Registers *)registers;

LLVMArch
~~~~~~~~

Subset actually simulated is ``x86`` and ``x86_64``. The enum lists
all LLVM targets (``arm``, ``aarch64``, ``mips``, ``riscv64``,
``wasm32``, ...) plus ``unknown`` for forward compatibility.

ProcessorResult
~~~~~~~~~~~~~~~

.. code-block:: c

   typedef enum {
     PROC_SUCCESS, PROC_BUFFER_OVERFLOW, PROC_BUFFER_UNDERFLOW,
     INVALID_INSTRUCTION, INVALID_REGISTER, INVALID_ADDRESS,
     PROC_DIVISION_BY_ZERO, PRIVILEGED_INSTRUCTION, PAGE_FAULT,
     PROC_SEGMENTATION_FAULT, ALIGNMENT_ERROR, FLOATING_POINT_ERROR,
     INTERRUPT, HALT, PROC_TIMEOUT, INVALID_OPERAND,
     PROC_STACK_OVERFLOW, PROC_STACK_UNDERFLOW, PROTECTION_FAULT,
     GENERAL_PROTECTION_FAULT, PROC_UNKNOWN_ERROR, NOT_INITIALIZED
   } ProcessorResult;

All simulation functions return ``NOT_INITIALIZED`` if ``cinit`` was
not called.

CommonOperator
~~~~~~~~~~~~~~

Normalized opcode produced by ``mapLLVMOpcodeToOperator`` in
``analyzer.cpp``: ``NOP``, ``MOV``, ``ADD``, ``SUB``, ``MUL``,
``DIV``, ``MOD``, ``INC``, ``DEC``, ``AND``, ``OR``, ``XOR``,
``NOT``, ``SHL``, ``SHR``, ``SAR``, ``CMP``, ``TEST``, ``JMP``,
``JE`` / ``JNE`` / ``JG`` / ``JGE`` / ``JL`` / ``JLE`` / ``JA`` /
``JAE`` / ``JB`` / ``JBE`` / ``JO`` / ``JNO`` / ``JS`` / ``JNS``,
``CALL``, ``RET``, ``PUSH``, ``POP``, ``LEA``, ``XCHG``,
``IMUL``, ``IDIV``, string ops (``MOVSB`` ... ``CMPSQ``),
``REP`` / ``REPE`` / ``REPNE``, ``LOCK``, ``XADD``,
``CMPXCHG``, ``SYSCALL`` / ``SYSENTER`` / ``SYSEXIT``,
``INT``, ``LOOP``, ``LEAVE`` / ``ENTER``, segment loads,
``CLD`` / ``STD`` / ``CLTD`` / ``CQO`` / ``CBW`` / ``CWDE`` /
``CDQE``, and ``UNKNOWN_OP``.

Data Movement
~~~~~~~~~~~~~

.. code-block:: c

   ProcessorResult copy(uintptr_t left, unsigned int loplen,
                        uintptr_t right, int roplen,
                        vector eop, vector eoplen);

``MOV`` simulation. Resolves register aliases via
``x86_64_setupRegister`` / ``x86_setupRegister``, checks
``loplen < roplen`` (``PROC_BUFFER_OVERFLOW``), then copies through
a ``falloc`` temporary. Updates no flags.

Arithmetic
~~~~~~~~~~

.. code-block:: c

   ProcessorResult add(uintptr_t left, unsigned int loplen,
                       uintptr_t right, int roplen,
                       vector eop, vector eoplen);
   ProcessorResult subtract(uintptr_t left, unsigned int loplen,
                            uintptr_t right, int roplen,
                            vector eop, vector eoplen);
   ProcessorResult multiply(uintptr_t left, unsigned int loplen,
                            uintptr_t right, int roplen,
                            vector eop, vector eoplen);
   ProcessorResult divide(uintptr_t left, unsigned int loplen,
                          uintptr_t right, int roplen,
                          vector eop, vector eoplen);
   ProcessorResult compare(uintptr_t left, unsigned int loplen,
                           uintptr_t right, int roplen,
                           vector eop, vector eoplen);
   ProcessorResult increment(uintptr_t left, unsigned int loplen);
   ProcessorResult decrement(uintptr_t left, unsigned int loplen);

``left`` is destination address, ``loplen`` / ``roplen`` are operand
sizes (1, 2, 4, 8). ``divide`` returns ``PROC_DIVISION_BY_ZERO`` on a
zero divisor. ``compare`` is a non-storing subtract. All update
``rflags`` / ``eflags``: ZF (0x40), PF (0x4), SF (0x80), CF (0x1),
OF (0x800).

.. code-block:: c

   uint64_t dst = 40, src = 2;
   add((uintptr_t)&dst, 8, (uintptr_t)&src, 8, emvec, emvec);
   subtract((uintptr_t)&dst, 8, (uintptr_t)&src, 8, emvec, emvec);

Logic, Shifts, Stack And Misc
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Declared in ``cominsset.h`` (see header for exact signatures):

.. code-block:: c

   ProcessorResult lgAnd(uintptr_t left, unsigned int loplen,
                         uintptr_t right, int roplen,
                         vector eops, vector eopslen);
   ProcessorResult lgOr(uintptr_t left, unsigned int loplen,
                        uintptr_t right, int roplen,
                        vector eops, vector eopslen);
   ProcessorResult lgXor(uintptr_t left, unsigned int loplen,
                         uintptr_t right, int roplen,
                         vector eops, vector eopslen);
   ProcessorResult lgNot(uintptr_t left, unsigned int loplen,
                         uintptr_t right, int roplen,
                         vector eops, vector eopslen);
   ProcessorResult shift_left(uintptr_t left, unsigned int loplen,
                              uintptr_t right, int roplen);
   ProcessorResult shift_right(uintptr_t left, unsigned int loplen,
                               uintptr_t right, int roplen);
   ProcessorResult push(uintptr_t left, unsigned int loplen);
   ProcessorResult pop(uintptr_t left, unsigned int loplen);
   ProcessorResult load_effective_address(uintptr_t left, unsigned int loplen,
                                          uintptr_t right, int roplen);
   ProcessorResult test_and(uintptr_t left, unsigned int loplen,
                            uintptr_t right, int roplen,
                            vector eops, vector eopslen);

analyzer.h - Code Analyzer
--------------------------

.. code-block:: c

   typedef enum {
     BUFFER_OVERFLOW, BUFFER_UNDERFLOW,
     OUT_OF_BOUNDS_READ, OUT_OF_BOUNDS_WRITE,
     USE_AFTER_FREE, DOUBLE_FREE, INVALID_FREE,
     NULL_POINTER_DEREFERENCE, MEMORY_LEAK, UNINITIALIZED_READ,
     DANGLING_POINTER_ACCESS, HEAP_CORRUPTION,
     DATA_RACE, DEADLOCK, LIVELOCK,
     MUTEX_UNLOCK_ERROR, THREAD_CREATION_FAILED, RESOURCE_STARVATION,
     DIVISION_BY_ZERO, MODULO_BY_ZERO,
     INTEGER_OVERFLOW, INTEGER_UNDERFLOW,
     FLOAT_INVALID_OPERATION, FLOAT_DIVISION_BY_ZERO,
     FLOAT_OVERFLOW, FLOAT_UNDERFLOW,
     TYPE_MISMATCH, INVALID_CAST,
     SEGMENTATION_FAULT, ILLEGAL_INSTRUCTION,
     STACK_OVERFLOW, STACK_UNDERFLOW, BUS_ERROR,
     PRIVILEGED_INSTRUCTION_VIOLATION, HARDWARE_TRAP, ALIGNMENT_FAULT,
     PERMISSION_DENIED, UNAUTHORIZED_SYSCALL,
     UNAUTHORIZED_FILE_ACCESS, UNAUTHORIZED_NETWORK_ACCESS,
     RESOURCE_LIMIT_EXCEEDED, SANDBOX_ESCAPE_ATTEMPT,
     MALFORMED_SYSCALL_ARGUMENT, POLICY_VIOLATION,
     PRIVILEGE_ESCALATION_ATTEMPT,
     FILE_NOT_FOUND, FILE_ACCESS_DENIED,
     EOF_REACHED_UNEXPECTEDLY, IO_OPERATION_FAILED, DISK_QUOTA_EXCEEDED,
     OKAY, TIMEOUT, CANCELLED, UNKNOWN_ERROR
   } bhResult;

   typedef struct {
     vector *riskyValue;
     vector *valueBehavor;
   } analyzedResult;

   void init(LLVMArch arch);
   analyzedResult analyzeFunction(dynvar functionName, dynvar source);

- ``init(arch)`` (``analyzer.cpp``, ``extern "C"``): initialize
  internal instruction / task queues. Call once.
- ``analyzeFunction(functionName, source)``: both args are ``dynvar``
  strings set with ``setValue``. ``source`` is a path to an ELF file,
  ``functionName`` an ELF symbol name. Parses ``.text`` with LIEF,
  disassembles with LLVM ``MCDisassembler`` (``x86_64-pc-linux-gnu``),
  queues heavy ops (``ADD`` / ``SUB`` / ``MUL`` / ``DIV`` / ``INC`` /
  ``DEC``) on worker threads, and returns ``analyzedResult`` with
  ``riskyValue`` / ``valueBehavor`` vectors (currently ``NULL``;
  AST nodes are freed after analysis).

Typical flow:

.. code-block:: c

   #include <analyzer.h>
   #include <dynvar.h>
   #include <insset/cominsset.h>

   cinit(x86_64);
   threadingInit();
   init(x86_64);

   dynvar fname = {0, 0}, src = {0, 0};
   setValue(&fname, "target_func");
   setValue(&src, "./libtarget.so");
   analyzedResult r = analyzeFunction(fname, src);

Errors (missing file, missing ``.text``, missing symbol, LLVM target
lookup failure) print to ``stderr`` and either return an empty result
or call ``bugDetected`` / ``exit(1)``.

insset/x86_64.h And insset/x86.h - Register Files
-------------------------------------------------

.. code-block:: c

   void x86_64_setupRegister(x86_64_Registers *regs, uintptr_t addr,
                             uintptr_t **op, unsigned int *oplen);
   void x86_setupRegister(x86_Registers *regs, uintptr_t addr,
                          uintptr_t **op, unsigned int *oplen);

Normalize a sub-register address to its canonical storage and size.
``addr`` is any address inside ``*regs``; on return ``*op`` points at
the canonical field and ``*oplen`` is its size. Calls ``bugDetected``
on an invalid offset. ``x86_64_Registers`` covers ``rax`` ... ``r15``
plus ``eax`` / ``ax`` / ``al`` family, ``rip`` / ``eip``,
``rflags`` / ``eflags``, segments, control / debug registers, FPU,
MMX, XMM / YMM / ZMM, ``mxcsr``, FS/GS base and common MSRs.
``x86_Registers`` is the 32-bit subset (``eax`` ... ``esp``,
``eip``, ``eflags``, 8 XMM registers).

You normally do not call these directly; ``copy`` / ``add`` / ...
call them when an operand falls inside ``registers``.

Complete Example
----------------

.. code-block:: c

   #include <string.h>
   #include <dynvar.h>
   #include <runtime.h>
   #include <threading.h>
   #include <analyzer.h>
   #include <insset/cominsset.h>

   int main() {
     cinit(x86_64);
     threadingInit();
     init(x86_64);

     vector v;
     vectorInit(&v, sizeof(int));
     lgr tmp;
     int n = 42;
     memcpy(tmp, &n, sizeof(n));
     vectorAppend(&v, tmp);

     dynvar fname = {0, 0}, src = {0, 0};
     setValue(&fname, "target_func");
     setValue(&src, "./libtarget.so");
     analyzedResult r = analyzeFunction(fname, src);

     vectorDeleteAll(&v);
     threadCleanup();
     return 0;
   }
