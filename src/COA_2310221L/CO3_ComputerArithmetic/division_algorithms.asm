; Restoring and non-restoring division on n-bit registers, giving the quotient and the remainder.

default rel

section .text
global restoring_divide, nonrestoring_divide

; restoring_divide(rdi = dividend Q, rsi = divisor M, rdx = n bits) -> rax = quotient, rdx = remainder (unsigned)
;   A (n+1 bits, starts 0) and Q are shifted left together; then A = A - M.
;   If A became negative the subtraction is undone (A = A + M, "restored") and the new bit of Q is 0, otherwise 1.
restoring_divide:
    push rbx
    push r12
    push r13
    mov r11d, edx                   ; cycles left
    mov ecx, edx
    mov r8, 1
    shl r8, cl
    dec r8                          ; mask of n bits
    lea r9, [r8 * 2 + 1]            ; mask of n+1 bits (the width of A)
    lea r10, [r8 + 1]               ; the sign bit of A
    mov r13, rdi
    and r13, r8                     ; Q = dividend
    mov r12, rsi
    and r12, r8                     ; M = divisor
    xor ebx, ebx                    ; A = 0
.cycle:
    shl rbx, 1
    mov rax, r8
    shr rax, 1
    inc rax                         ; the top bit of Q
    test r13, rax
    jz .no_top_bit
    or rbx, 1                       ; the top bit of Q moves into the bottom of A
.no_top_bit:
    and rbx, r9
    shl r13, 1
    and r13, r8
    sub rbx, r12                    ; A = A - M
    and rbx, r9
    test rbx, r10
    jz .positive
    add rbx, r12                    ; negative: restore A
    and rbx, r9
    jmp .next
.positive:
    or r13, 1                       ; the new quotient bit is 1
.next:
    dec r11d
    jnz .cycle
    mov rax, r13
    mov rdx, rbx
    pop r13
    pop r12
    pop rbx
    ret

; nonrestoring_divide(rdi = dividend Q, rsi = divisor M, rdx = n bits) -> rax = quotient, rdx = remainder (unsigned)
;   no undo step: if A was negative before the shift then A = 2A + M, otherwise A = 2A - M;
;   the new bit of Q is 1 when A is not negative afterwards. A final correction adds M if A ended negative.
nonrestoring_divide:
    push rbx
    push r12
    push r13
    push r14
    mov r11d, edx
    mov ecx, edx
    mov r8, 1
    shl r8, cl
    dec r8
    lea r9, [r8 * 2 + 1]
    lea r10, [r8 + 1]
    mov r13, rdi
    and r13, r8
    mov r12, rsi
    and r12, r8
    xor ebx, ebx
.cycle:
    mov r14, rbx
    and r14, r10                    ; r14 is not 0 when A is negative before the shift
    shl rbx, 1
    mov rax, r8
    shr rax, 1
    inc rax
    test r13, rax
    jz .no_top_bit
    or rbx, 1
.no_top_bit:
    and rbx, r9
    shl r13, 1
    and r13, r8
    test r14, r14
    jnz .add_divisor
    sub rbx, r12                    ; A was not negative: subtract M
    jmp .set_bit
.add_divisor:
    add rbx, r12                    ; A was negative: add M
.set_bit:
    and rbx, r9
    test rbx, r10
    jnz .next                       ; negative now: the quotient bit stays 0
    or r13, 1
.next:
    dec r11d
    jnz .cycle
    test rbx, r10
    jz .done
    add rbx, r12                    ; the remainder must not be negative: correct it
    and rbx, r9
.done:
    mov rax, r13
    mov rdx, rbx
    pop r14
    pop r13
    pop r12
    pop rbx
    ret
