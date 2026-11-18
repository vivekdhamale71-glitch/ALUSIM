; Booth's signed multiplication and add-and-shift unsigned multiplication on n-bit registers.

default rel

section .text
global booth_multiply, add_shift_multiply

; booth_multiply(rdi = M multiplicand, rsi = Q multiplier, rdx = n bits, 2..30) -> rax : signed M * Q
;   registers: A (accumulator, starts 0), Q (multiplier), Q-1 (an extra bit, starts 0), M (multiplicand)
;   every cycle looks at the last bit of Q and Q-1:  10 -> A = A - M,  01 -> A = A + M,  00 and 11 -> nothing
;   then A, Q and Q-1 are shifted right together (the sign bit of A is kept: an arithmetic shift)
;   M must not be -2^(n-1): that value cannot be negated in n bits (use one more bit)
booth_multiply:
    push rbx
    push r12
    push r13
    mov r11d, edx                   ; r11 = number of cycles left
    mov ecx, edx
    mov r8, 1
    shl r8, cl
    dec r8                          ; r8 = mask of n bits
    lea ecx, [rdx - 1]
    mov r9, 1
    shl r9, cl                      ; r9 = the sign bit of an n-bit register
    mov r10, rdi
    and r10, r8                     ; r10 = M
    mov rbx, rsi
    and rbx, r8                     ; rbx = Q
    xor r12d, r12d                  ; r12 = A
    xor r13d, r13d                  ; r13 = Q-1
.cycle:
    mov rax, rbx
    and eax, 1
    shl eax, 1
    or rax, r13                     ; eax = (last bit of Q, Q-1)
    cmp eax, 2
    je .subtract
    cmp eax, 1
    je .add
    jmp .shift
.subtract:
    sub r12, r10                    ; A = A - M
    and r12, r8
    jmp .shift
.add:
    add r12, r10                    ; A = A + M
    and r12, r8
.shift:
    mov r13, rbx
    and r13d, 1                     ; the old last bit of Q becomes Q-1
    shr rbx, 1
    test r12, 1
    jz .keep_sign
    or rbx, r9                      ; the bit that falls out of A enters the top of Q
.keep_sign:
    mov rax, r12
    and rax, r9                     ; the sign bit of A stays where it is
    shr r12, 1
    or r12, rax
    dec r11d
    jnz .cycle
    mov rax, r12                    ; the product is A and Q written side by side (2n bits)
    mov ecx, edx
    shl rax, cl
    or rax, rbx
    mov ecx, 64
    sub ecx, edx
    sub ecx, edx
    shl rax, cl                     ; move the 2n-bit sign bit to bit 63 ...
    sar rax, cl                     ; ... and back, which sign-extends the product
    pop r13
    pop r12
    pop rbx
    ret

; add_shift_multiply(rdi = M, rsi = Q, rdx = n bits) -> rax : unsigned M * Q
;   every cycle: if the last bit of Q is 1 then A = A + M (the carry goes into C);
;   then C, A and Q are shifted right together
add_shift_multiply:
    push rbx
    push r12
    push r13
    mov r11d, edx
    mov ecx, edx
    mov r8, 1
    shl r8, cl
    dec r8                          ; mask of n bits
    lea r9, [r8 + 1]                ; 2^n: the bit a carry lands in
    mov r10, rdi
    and r10, r8                     ; M
    mov rbx, rsi
    and rbx, r8                     ; Q
    xor r12d, r12d                  ; A
    xor r13d, r13d                  ; C
.cycle:
    test rbx, 1
    jz .shift
    add r12, r10                    ; A = A + M
    test r12, r9
    setnz r13b                      ; C = the carry out of the n-bit addition
    and r12, r8
.shift:
    shr rbx, 1
    test r12, 1
    jz .move_a
    lea rax, [r8 + 1]
    shr rax, 1
    or rbx, rax                     ; the bit falling out of A enters the top of Q
.move_a:
    shr r12, 1
    test r13, r13
    jz .next
    lea rax, [r8 + 1]
    shr rax, 1
    or r12, rax                     ; C enters the top of A
.next:
    xor r13d, r13d
    dec r11d
    jnz .cycle
    mov rax, r12                    ; the product is A and Q side by side
    mov ecx, edx
    shl rax, cl
    or rax, rbx
    pop r13
    pop r12
    pop rbx
    ret
