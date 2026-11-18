; Immediate, register, direct, indirect, based and scaled-index addressing, LEA, and the stack with PUSH, POP, CALL and RET.

default rel

section .data
value: dq 777
table: dq 10, 20, 30, 40, 50

section .text
global mode_immediate, mode_register, mode_direct, mode_register_indirect, mode_based_displacement
global mode_scaled_index, mode_base_index_displacement, address_of_element, swap_through_stack, quadruple

; mode_immediate() -> rax = 42 : the value is part of the instruction itself
mode_immediate:
    mov eax, 42
    ret

; mode_register(rdi = value) -> rax : the operand is in a register
mode_register:
    mov rax, rdi
    ret

; mode_direct() -> rax = value : the address of the variable is fixed in the instruction
mode_direct:
    mov rax, [value]
    ret

; mode_register_indirect(rdi = address) -> rax : the register holds the address of the operand
mode_register_indirect:
    mov rax, [rdi]
    ret

; mode_based_displacement(rdi = base address) -> rax : base register + a constant, [rdi + 16]
mode_based_displacement:
    mov rax, [rdi + 16]
    ret

; mode_scaled_index(rdi = array, rsi = index) -> rax : array[index] for 8-byte elements, [rdi + rsi*8]
mode_scaled_index:
    mov rax, [rdi + rsi * 8]
    ret

; mode_base_index_displacement(rdi = array, rsi = index) -> rax : array[index + 1], [rdi + rsi*8 + 8]
mode_base_index_displacement:
    mov rax, [rdi + rsi * 8 + 8]
    ret

; address_of_element(rdi = array, rsi = index) -> rax : LEA computes the address and reads no memory
address_of_element:
    lea rax, [rdi + rsi * 8]
    ret

; swap_through_stack(rdi = a, rsi = b) -> rax = b, rdx = a : PUSH puts on the stack, POP takes off (last in, first out)
swap_through_stack:
    push rdi
    push rsi
    pop rax                         ; gets rsi, the value pushed last
    pop rdx                         ; gets rdi
    ret

; quadruple(rdi = value) -> rax : CALL pushes the return address, RET pops it and jumps back
quadruple:
    mov rax, rdi
    call double_rax
    call double_rax
    ret

double_rax:
    add rax, rax
    ret
