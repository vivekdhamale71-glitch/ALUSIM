; A control store with a micro-program counter and a dispatch table that runs the micro-routine of an opcode.

default rel

; micro-operations: one bit each (horizontal micro-code)
U_MAR_PC      equ 1 << 0
U_MDR_MEM     equ 1 << 1
U_PC_INC      equ 1 << 2
U_IR_MDR      equ 1 << 3
U_MAR_ADDR    equ 1 << 4
U_RT_MDR      equ 1 << 5
U_MDR_RT      equ 1 << 6
U_MEM_MDR     equ 1 << 7
U_Y_RS        equ 1 << 8
U_Z_ADD       equ 1 << 9
U_RD_Z        equ 1 << 10
U_Z_SUB       equ 1 << 11
U_BR_ZERO     equ 1 << 12
U_PC_TARGET   equ 1 << 13
U_Z_ADDIMM    equ 1 << 14
U_RT_Z        equ 1 << 15

; the sequencing field of a micro-instruction says where the micro-program counter goes next
SEQ_NEXT      equ 0                 ; the next word of the control store
SEQ_DISPATCH  equ 1                 ; jump to the routine of the opcode (through the dispatch table)
SEQ_FETCH     equ 2                 ; the instruction is finished: start again with the fetch

%macro MICRO 2                      ; one control store word = micro-operations + sequencing << 24
    dq (%1) | ((%2) << 24)
%endmacro

section .rodata
align 8
control_store:
    MICRO U_MAR_PC,               SEQ_NEXT       ; 0  fetch: MAR <- PC
    MICRO U_MDR_MEM | U_PC_INC,   SEQ_NEXT       ; 1  MDR <- memory[MAR], PC <- PC + 1
    MICRO U_IR_MDR,               SEQ_DISPATCH   ; 2  IR <- MDR, then branch on the opcode
    MICRO U_MAR_ADDR,             SEQ_NEXT       ; 3  LW:   MAR <- address
    MICRO U_MDR_MEM,              SEQ_NEXT       ; 4        MDR <- memory[MAR]
    MICRO U_RT_MDR,               SEQ_FETCH      ; 5        register <- MDR
    MICRO U_MAR_ADDR,             SEQ_NEXT       ; 6  SW:   MAR <- address
    MICRO U_MDR_RT,               SEQ_NEXT       ; 7        MDR <- register
    MICRO U_MEM_MDR,              SEQ_FETCH      ; 8        memory[MAR] <- MDR
    MICRO U_Y_RS,                 SEQ_NEXT       ; 9  R-type: Y <- first register
    MICRO U_Z_ADD,                SEQ_NEXT       ; 10       Z <- Y + second register
    MICRO U_RD_Z,                 SEQ_FETCH      ; 11       destination <- Z
    MICRO U_Z_SUB,                SEQ_NEXT       ; 12 BEQ:  Z <- register - register
    MICRO U_BR_ZERO,              SEQ_FETCH      ; 13       if Z = 0 then PC <- target
    MICRO U_PC_TARGET,            SEQ_FETCH      ; 14 JMP:  PC <- target
    MICRO U_Y_RS,                 SEQ_NEXT       ; 15 ADDI: Y <- register
    MICRO U_Z_ADDIMM,             SEQ_NEXT       ; 16       Z <- Y + constant
    MICRO U_RT_Z,                 SEQ_FETCH      ; 17       register <- Z

; the dispatch table: opcode (0 R, 1 LW, 2 SW, 3 BEQ, 4 JMP, 5 ADDI) -> first word of its routine
dispatch_table:
    dq 9, 3, 6, 12, 14, 15

section .text
global micro_cycles, micro_operations_of

; micro_cycles(rdi = opcode 0..5) -> rax : how many clock cycles the instruction needs, one per control store word
;   starts at word 0 (the fetch); every word names the next one in its sequencing field
micro_cycles:
    lea r8, [rel control_store]
    lea r9, [rel dispatch_table]
    xor ecx, ecx                    ; micro-program counter
    xor eax, eax                    ; cycles
.cycle:
    inc rax
    mov rdx, [r8 + rcx * 8]         ; the micro-instruction register
    shr rdx, 24
    and edx, 3                      ; the sequencing field
    cmp edx, SEQ_FETCH
    je .finished
    cmp edx, SEQ_DISPATCH
    je .dispatch
    inc rcx                         ; SEQ_NEXT
    jmp .cycle
.dispatch:
    mov rcx, [r9 + rdi * 8]         ; the routine of this opcode
    jmp .cycle
.finished:
    ret

; micro_operations_of(rdi = micro-program counter 0..17) -> rax : the micro-operation bits of that control store word
micro_operations_of:
    lea r8, [rel control_store]
    mov rax, [r8 + rdi * 8]
    and eax, 0FFFFh
    ret
