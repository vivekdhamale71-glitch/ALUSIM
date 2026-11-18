; Signed and unsigned comparison, conditional moves, absolute value and safe division with CQO and IDIV.

default rel

section .text
global less_signed, below_unsigned, max_of_two, absolute_value, safe_divide

; less_signed(rdi = a, rsi = b) -> eax : 1 if a < b treating both as SIGNED numbers (the JL condition)
less_signed:
    cmp rdi, rsi                    ; sets SF, OF, CF, ZF from a - b
    setl al                         ; "less": SF is different from OF
    movzx eax, al
    ret

; below_unsigned(rdi = a, rsi = b) -> eax : 1 if a < b treating both as UNSIGNED numbers (the JB condition)
below_unsigned:
    cmp rdi, rsi
    setb al                         ; "below": the subtraction needed a borrow (CF = 1)
    movzx eax, al
    ret

; max_of_two(rdi = a, rsi = b) -> rax : the larger of two signed numbers, without a jump
max_of_two:
    mov rax, rdi
    cmp rdi, rsi
    cmovl rax, rsi                  ; conditional move: copy b only if a < b
    ret

; absolute_value(rdi = n) -> rax : |n|
absolute_value:
    mov rax, rdi
    neg rax                         ; -n; the sign flag tells whether n was positive
    cmovs rax, rdi                  ; -n is negative, so n was positive: use n
    ret

; safe_divide(rdi = dividend, rsi = divisor, rdx = where to store the quotient) -> eax : 0 = ok, 1 = division by zero
;   IDIV divides the 128-bit pair RDX:RAX, so CQO first copies the sign of RAX into every bit of RDX
safe_divide:
    test rsi, rsi
    jz .by_zero                     ; a real IDIV would raise a divide error
    mov r8, rdx                     ; keep the output address (idiv uses rdx)
    mov rax, rdi
    cqo                             ; sign-extend rax into rdx:rax
    idiv rsi                        ; rax = quotient, rdx = remainder
    mov [r8], rax
    xor eax, eax
    ret
.by_zero:
    mov eax, 1
    ret
