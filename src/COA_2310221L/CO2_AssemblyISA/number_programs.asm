; Factorial, Fibonacci, GCD, power, prime test and bit count.

default rel

section .text
global factorial, fibonacci, gcd, power, is_prime, count_set_bits

; factorial(rdi = n) -> rax : n! computed recursively, n * (n-1)!
factorial:
    cmp rdi, 1
    jg .recurse
    mov eax, 1                      ; 0! = 1! = 1
    ret
.recurse:
    push rdi                        ; keep n on the stack while the smaller factorial is computed
    dec rdi
    call factorial
    pop rdi
    imul rax, rdi
    ret

; fibonacci(rdi = n) -> rax : the n-th Fibonacci number (0, 1, 1, 2, 3, 5, ...)
fibonacci:
    xor eax, eax                    ; a = F(0)
    mov ecx, 1                      ; b = F(1)
.next:
    test rdi, rdi
    jz .done
    lea rdx, [rax + rcx]            ; next = a + b
    mov rax, rcx
    mov rcx, rdx
    dec rdi
    jmp .next
.done:
    ret

; gcd(rdi = a, rsi = b) -> rax : Euclid's method, gcd(a, b) = gcd(b, a mod b)
gcd:
    mov rax, rdi
    mov rcx, rsi
.again:
    test rcx, rcx
    jz .done
    xor edx, edx
    div rcx                         ; rdx = a mod b
    mov rax, rcx
    mov rcx, rdx
    jmp .again
.done:
    ret

; power(rdi = base, rsi = exponent) -> rax : square and multiply, about log2(exponent) steps
power:
    mov eax, 1
    mov rcx, rdi
.next:
    test rsi, rsi
    jz .done
    test sil, 1
    jz .square
    imul rax, rcx                   ; this bit of the exponent is 1: multiply it in
.square:
    imul rcx, rcx
    shr rsi, 1
    jmp .next
.done:
    ret

; is_prime(rdi = n) -> rax : 1 if n is prime, else 0 (trial division up to the square root)
is_prime:
    cmp rdi, 2
    jb .no
    mov ecx, 2
.try:
    mov rax, rcx
    imul rax, rcx
    cmp rax, rdi
    ja .yes                         ; divisor squared passed n: no divisor exists
    mov rax, rdi
    xor edx, edx
    div rcx
    test rdx, rdx
    jz .no                          ; divides evenly: not prime
    inc rcx
    jmp .try
.yes:
    mov eax, 1
    ret
.no:
    xor eax, eax
    ret

; count_set_bits(rdi = n) -> rax : how many bits of n are 1
count_set_bits:
    xor eax, eax
.next:
    test rdi, rdi
    jz .done
    mov rcx, rdi
    and ecx, 1
    add rax, rcx
    shr rdi, 1
    jmp .next
.done:
    ret
