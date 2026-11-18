; String length, reverse, palindrome test, vowel count and upper-casing.

default rel

section .text
global string_length, reverse_string, is_palindrome, count_vowels, to_upper

; string_length(rdi = zero-terminated text) -> rax : number of characters before the 0 byte
string_length:
    xor eax, eax
.next:
    cmp byte [rdi + rax], 0
    je .done
    inc rax
    jmp .next
.done:
    ret

; reverse_string(rdi = text) : reverses it in place with two pointers that meet in the middle
reverse_string:
    call string_length
    lea rsi, [rdi + rax - 1]        ; right end
    mov rdx, rdi                    ; left end
.next:
    cmp rdx, rsi
    jae .done
    mov al, [rdx]
    mov cl, [rsi]
    mov [rdx], cl                   ; swap the two characters
    mov [rsi], al
    inc rdx
    dec rsi
    jmp .next
.done:
    ret

; is_palindrome(rdi = text) -> rax : 1 if it reads the same backwards, else 0
is_palindrome:
    call string_length
    lea rsi, [rdi + rax - 1]
    mov rdx, rdi
.next:
    cmp rdx, rsi
    jae .yes
    mov al, [rdx]
    cmp al, [rsi]
    jne .no
    inc rdx
    dec rsi
    jmp .next
.yes:
    mov eax, 1
    ret
.no:
    xor eax, eax
    ret

; count_vowels(rdi = text) -> rax : how many of a, e, i, o, u (either case) it contains
count_vowels:
    xor eax, eax
    mov rsi, rdi
.next:
    movzx ecx, byte [rsi]
    test cl, cl
    jz .done
    or cl, 20h                      ; ASCII trick: forces a capital letter to lower case
    cmp cl, 'a'
    je .vowel
    cmp cl, 'e'
    je .vowel
    cmp cl, 'i'
    je .vowel
    cmp cl, 'o'
    je .vowel
    cmp cl, 'u'
    jne .skip
.vowel:
    inc rax
.skip:
    inc rsi
    jmp .next
.done:
    ret

; to_upper(rdi = text) : turns every small letter into a capital letter, in place
to_upper:
    mov rsi, rdi
.next:
    mov al, [rsi]
    test al, al
    jz .done
    cmp al, 'a'
    jb .skip
    cmp al, 'z'
    ja .skip
    sub al, 20h                     ; 'a' (97) - 32 = 'A' (65)
    mov [rsi], al
.skip:
    inc rsi
    jmp .next
.done:
    ret
