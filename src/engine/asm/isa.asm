; =============================================================================
;  2310221L.CO2  Instruction semantics of the simulated x86-64 ISA
;  File : isa.asm   (NASM, linked into the simulator, called from C++)
;
;  int vas_condition(int cc, uint64_t rflags)
;      Is the branch taken?  cc: 0 always (JMP/CALL/RET) 1 JE 2 JNE 3 JG
;      4 JGE 5 JL 6 JLE.  The simulated flags are loaded into RFLAGS and the
;      real SETcc instruction decides.
;
;  int vas_idiv(uint64_t rdx, uint64_t rax, int64_t divisor, int64_t out[2])
;      IDIV: signed RDX:RAX / divisor -> out[0] quotient, out[1] remainder.
;      returns 0 ok, 1 divide by zero, 2 quotient does not fit in 64 bits.
;      Both errors are #DE on real hardware, so they are detected first and
;      the real IDIV only runs when it cannot trap.
;
;  int64_t vas_cqo(int64_t rax)
;      CQO: the value RDX gets (RAX sign-extended), 0 or -1.
; =============================================================================

%include "engine/asm/abi.inc"

%define STATUS_MASK 0x8C1

section .rodata
align 4
cond_table:                         ; offsets from the table (position independent)
    dd cc_always - cond_table, cc_e - cond_table, cc_ne - cond_table, cc_g - cond_table
    dd cc_ge - cond_table, cc_l - cond_table, cc_le - cond_table
COND_COUNT equ ($ - cond_table) / 4

section .text
global vas_condition
global vas_idiv
global vas_cqo

; -----------------------------------------------------------------------------
vas_condition:
    ENTRY
    xor  eax, eax
    cmp  edi, COND_COUNT
    jae  cond_out                     ; unknown condition: not taken
    lea  r8, [rel cond_table]
    movsxd r9, dword [r8 + rdi*4]
    add  r8, r9
    and  rsi, STATUS_MASK
    or   rsi, 2
    push rsi
    popfq                           ; simulated flags -> real RFLAGS
    jmp  r8
cc_always:
    mov  al, 1
    jmp  cond_out
cc_e:
    sete  al                        ; ZF = 1
    jmp  cond_out
cc_ne:
    setne al                        ; ZF = 0
    jmp  cond_out
cc_g:
    setg  al                        ; ZF = 0 and SF = OF
    jmp  cond_out
cc_ge:
    setge al                        ; SF = OF
    jmp  cond_out
cc_l:
    setl  al                        ; SF != OF
    jmp  cond_out
cc_le:
    setle al                        ; ZF = 1 or SF != OF
cond_out:
    movzx eax, al
    RETURN

; -----------------------------------------------------------------------------
; rdi = high (RDX), rsi = low (RAX), rdx = divisor, rcx = out
vas_idiv:
    ENTRY
    push rbx
    mov  r8, rdi                    ; keep the original dividend
    mov  r9, rsi
    mov  r10, rdx                   ; divisor
    mov  r11, rcx                   ; out pointer
    test r10, r10
    jz   .div_zero

    ; --- magnitude of the 128-bit dividend in rdi:rsi ---------------------
    mov  rbx, rdi
    shr  rbx, 63                    ; rbx = sign of the dividend
    test rbx, rbx
    jz   .dividend_pos
    not  rdi                        ; two's complement of 128 bits
    not  rsi
    add  rsi, 1
    adc  rdi, 0
.dividend_pos:
    ; --- magnitude of the divisor ------------------------------------------
    mov  rcx, r10
    mov  rax, r10
    shr  rax, 63
    xor  rbx, rax                   ; rbx = sign of the quotient
    test rax, rax
    jz   .divisor_pos
    neg  rcx                        ; INT64_MIN stays 2^63 (unsigned)
.divisor_pos:
    cmp  rdi, rcx
    jae  .overflow                  ; unsigned quotient >= 2^64
    mov  rdx, rdi
    mov  rax, rsi
    div  rcx                        ; rax = |quotient|
    mov  rdx, 0x8000000000000000
    test rbx, rbx
    jnz  .negative
    cmp  rax, rdx
    jae  .overflow                  ; positive quotient must be < 2^63
    jmp  .safe
.negative:
    cmp  rax, rdx
    ja   .overflow                  ; negative quotient may be -2^63
.safe:
    mov  rdx, r8
    mov  rax, r9
    idiv r10                        ; the real instruction, cannot trap now
    mov  [r11], rax                 ; quotient
    mov  [r11 + 8], rdx             ; remainder (same sign as the dividend)
    xor  eax, eax
    jmp  .ret
.div_zero:
    mov  eax, 1
    jmp  .ret
.overflow:
    mov  eax, 2
.ret:
    pop  rbx
    RETURN

; -----------------------------------------------------------------------------
vas_cqo:
    ENTRY
    mov  rax, rdi
    cqo                             ; RDX = all copies of RAX's sign bit
    mov  rax, rdx
    RETURN

SECTION_NOTE
