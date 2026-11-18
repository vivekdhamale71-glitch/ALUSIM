; An ALU routine that runs ADD, SUB, AND, OR, XOR, NOT, NEG, INC, DEC, shifts, multiply and divide and returns the flags.

default rel

STATUS_MASK equ 0x8C1               ; the four flags that are kept: OF (bit 11), SF (7), ZF (6), CF (0)

section .rodata
align 4
alu_table:                          ; offsets of the 15 operations, indexed by the operation number
    dd op_none - alu_table, op_pass - alu_table, op_add - alu_table, op_sub - alu_table
    dd op_mul - alu_table,  op_div - alu_table,  op_and - alu_table, op_or - alu_table
    dd op_xor - alu_table,  op_not - alu_table,  op_neg - alu_table, op_shl - alu_table
    dd op_shr - alu_table,  op_inc - alu_table,  op_dec - alu_table
ALU_OPERATIONS equ ($ - alu_table) / 4

section .text
global alu

; alu(rdi = operation 0..14, rsi = a, rdx = b, rcx = pointer to the 64-bit flags) -> rax = result
;   the simulated flags are loaded into the real RFLAGS, the real instruction runs, and the
;   flags it produced are written back, so ZF, SF, CF and OF behave exactly like x86
alu:
    mov r8, rcx                     ; r8 = where the flags are kept
    mov r9, [r8]
    and r9, STATUS_MASK
    or r9, 2                        ; bit 1 of RFLAGS is always 1
    cmp edi, ALU_OPERATIONS
    jae op_none
    cmp edi, 5
    je op_div
    lea r10, [rel alu_table]
    movsxd r11, dword [r10 + rdi * 4]
    add r10, r11                    ; the address of the chosen operation
    mov rcx, rdx                    ; shift counts are taken from CL
    mov rax, rsi                    ; rax = a
    push r9
    popfq                           ; load the simulated flags
    jmp r10                         ; a jump does not change the flags

op_none:
    mov rax, rsi
    jmp keep_flags
op_pass:
    mov rax, rdx                    ; MOV: the result is b, flags unchanged
    jmp finish
op_add:
    add rax, rdx
    jmp finish
op_sub:
    sub rax, rdx                    ; CMP uses this too and throws the result away
    jmp finish
op_and:
    and rax, rdx
    jmp finish
op_or:
    or rax, rdx
    jmp finish
op_xor:
    xor rax, rdx
    jmp finish
op_not:
    not rax                         ; NOT changes no flags
    jmp finish
op_neg:
    neg rax
    jmp finish
op_inc:
    inc rax                         ; INC and DEC leave the carry flag alone
    jmp finish
op_dec:
    dec rax
    jmp finish
op_shl:
    shl rax, cl
    jmp finish
op_shr:
    shr rax, cl
    jmp finish
op_mul:
    imul rax, rdx                   ; CF and OF say whether the product fitted in 64 bits
    jmp finish

; signed division a / b; a zero divisor and INT64_MIN / -1 are handled without trapping
op_div:
    mov rax, rsi
    test rdx, rdx
    jz keep_flags
    mov r10, rdx
    cmp r10, -1
    jne divide
    mov r11, 0x8000000000000000
    cmp rax, r11
    je keep_flags
divide:
    cqo                             ; sign-extend rax into rdx:rax
    idiv r10
    jmp keep_flags

finish:
    pushfq                          ; read back the flags the real instruction produced
    pop r10
    and r10, STATUS_MASK
    or r10, 2
    mov [r8], r10
    ret
keep_flags:
    mov [r8], r9                    ; the operation leaves the flags as they were
    ret
