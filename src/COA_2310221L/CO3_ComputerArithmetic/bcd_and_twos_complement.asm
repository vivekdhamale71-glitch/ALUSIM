; Two's complement, binary to BCD and back, BCD addition, and the carry and overflow flags.

default rel

section .text
global twos_complement, bin_to_bcd, bcd_to_bin, bcd_add, add_carry_and_overflow

; twos_complement(rdi = x, rsi = n bits) -> rax : the n-bit pattern of -x: invert every bit, then add 1
twos_complement:
    mov ecx, esi
    mov rdx, 1
    shl rdx, cl
    dec rdx                         ; mask of n bits
    mov rax, rdi
    not rax                         ; invert
    inc rax                         ; add one
    and rax, rdx
    ret

; bin_to_bcd(rdi = number) -> rax : packed BCD, one decimal digit in every 4 bits (1234 becomes 0x1234)
bin_to_bcd:
    mov rax, rdi
    xor r8d, r8d                    ; the packed result
    xor ecx, ecx                    ; bit position of the next digit
    mov r9d, 10
.digit:
    xor edx, edx
    div r9                          ; rdx = the last decimal digit, rax = the rest
    shl rdx, cl
    or r8, rdx
    add ecx, 4
    test rax, rax
    jz .done
    cmp ecx, 64
    jb .digit
.done:
    mov rax, r8
    ret

; bcd_to_bin(rdi = packed BCD) -> rax : the number: value = value * 10 + digit, from the top digit down
bcd_to_bin:
    xor eax, eax
    mov ecx, 60
.digit:
    mov rdx, rdi
    shr rdx, cl
    and edx, 0Fh
    imul rax, rax, 10
    add rax, rdx
    sub ecx, 4
    jns .digit
    ret

; bcd_add(rdi = packed BCD a, rsi = packed BCD b) -> rax : packed BCD a + b
;   digit by digit: sum = a + b + carry; if the sum is more than 9 add 6 (it skips the six unused
;   codes A..F) and carry 1 into the next digit
bcd_add:
    xor eax, eax
    xor r8d, r8d                    ; carry
    xor ecx, ecx
.digit:
    mov rdx, rdi
    shr rdx, cl
    and edx, 0Fh
    mov r9, rsi
    shr r9, cl
    and r9d, 0Fh
    add edx, r9d
    add edx, r8d
    xor r8d, r8d
    cmp edx, 9
    jbe .store
    add edx, 6                      ; decimal correction
    and edx, 0Fh
    mov r8d, 1
.store:
    shl rdx, cl
    or rax, rdx
    add ecx, 4
    cmp ecx, 64
    jb .digit
    ret

; add_carry_and_overflow(edi = a, esi = b, both 8-bit) -> eax : bit 0 = carry (unsigned overflow), bit 1 = overflow (signed)
;   200 + 100 carries but does not overflow; 100 + 50 overflows but does not carry
add_carry_and_overflow:
    mov al, dil
    add al, sil
    setc cl                         ; carry flag
    seto dl                         ; overflow flag
    movzx eax, cl
    movzx edx, dl
    shl edx, 1
    or eax, edx
    ret
