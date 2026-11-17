# 2310221L.CO2 — Develop assembly language programs  [L3]

**Files:**
- `Instruction.h / .cpp` — the instruction set: 33 x86 instructions (MOV, ADD, SUB, IMUL, IDIV, CQO, AND/OR/XOR/NOT, SHL/SHR, CMP/TEST, Jcc, JMP, CALL/RET, PUSH/POP, NOP, HLT)
- `Assembler.h / .cpp` — two-pass assembler (labels, `section .data`, `dq`, memory operands)

English and C++ input are compiled into genuine assembly; programs can also be written directly in assembly.

**See it in the app:** Compiler → Generated Assembly · Assembly mode · samples "ASM: …".
