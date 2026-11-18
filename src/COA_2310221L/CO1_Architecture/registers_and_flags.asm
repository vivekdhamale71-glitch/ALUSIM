; The register views EAX, AX, AH and AL, and the flags CF, ZF, SF and OF after an addition or subtraction.

default rel

section .text
global view_eax, view_ax, view_ah, view_al, write_eax, write_ax
global flags_after_add, flags_after_sub, flag_is_set

; view_eax(rdi = value) -> rax : the low 32 bits of the register (the 80386 view EAX)
view_eax:
    mov eax, edi                    ; writing a 32-bit register also clears bits 63..32 of rax
    ret

; view_ax(rdi = value) -> rax : the low 16 bits (the 8086 view AX)
view_ax:
    movzx eax, di
    ret

; view_al(rdi = value) -> rax : the low 8 bits (AL)
view_al:
    movzx eax, dil
    ret

; view_ah(rdi = value) -> rax : bits 8..15 (AH): shift right by 8, keep 8 bits
view_ah:
    mov rax, rdi
    shr rax, 8
    and eax, 0FFh
    ret

; write_eax(rdi = old rax, rsi = new value) -> rax : a 32-bit write CLEARS bits 63..32
write_eax:
    mov rax, rdi
    mov eax, esi
    ret

; write_ax(rdi = old rax, rsi = new value) -> rax : a 16-bit write keeps bits 63..16 unchanged
write_ax:
    mov rax, rdi
    mov ax, si
    ret

; flags_after_add(rdi = a, rsi = b) -> rax : the RFLAGS register right after a + b
flags_after_add:
    add rdi, rsi
    pushfq                          ; copy the flags onto the stack ...
    pop rax                         ; ... and into rax
    ret

; flags_after_sub(rdi = a, rsi = b) -> rax : the RFLAGS register right after a - b
flags_after_sub:
    sub rdi, rsi
    pushfq
    pop rax
    ret

; flag_is_set(rdi = rflags, esi = bit number) -> eax : 1 if that flag bit is 1
;   CF (carry) bit 0, ZF (zero) bit 6, SF (sign) bit 7, OF (overflow) bit 11
flag_is_set:
    mov rax, rdi
    mov ecx, esi
    shr rax, cl
    and eax, 1
    ret
