; =============================================================================
;  2310221L.CO3  Computer arithmetic algorithms used by the Arithmetic screen
;  File : arithmetic.asm   (NASM, linked into the simulator, called from C++)
;
;  Each algorithm runs on n-bit registers (n = 2..30) and records every step
;  into an array of 32-byte records that the screen animates:
;      struct ArithStepRec { uint64 A, Q, extra, kind; }
;      extra = Q-1 (Booth) or C (add-and-shift);  kind = one of STEP_*
;  Each returns the number of records written (at most 1 + 3n).
;
;  int vas_booth(int64 M, int64 Q, int n, ArithStepRec* out)       signed  M x Q
;  int vas_add_shift(uint64 M, uint64 Q, int n, ArithStepRec* out) unsigned M x Q
;  int vas_restoring_div(uint64 Q, uint64 M, int n, ArithStepRec* out)
;          unsigned Q / M with an (n+1)-bit A; M = 0 writes nothing, returns 0
;  uint64 vas_bin_to_bcd(uint64 v)    binary -> packed BCD (up to 16 digits)
;  uint64 vas_bcd_to_bin(uint64 bcd)  packed BCD -> binary
; =============================================================================

%include "COA_2310221L/abi.inc"

%define STEP_INIT     0
%define STEP_SUB      1             ; A = A - M
%define STEP_ADD      2             ; A = A + M
%define STEP_NOP      3             ; Booth 00/11 or Q0 = 0: no add
%define STEP_SHR      4             ; shift right
%define STEP_SHL      5             ; shift left
%define STEP_RESTORE  6             ; A < 0  : A = A + M, Q0 = 0
%define STEP_SETQ     7             ; A >= 0 : Q0 = 1

; registers shared by the three algorithms
;   rcx out   rax records   r8 mask   r9 M   r10 A   r11 Q   r12 extra
;   r13 loop counter   r14 top bit 2^(n-1)   r15 n
%macro RECORD 1
    mov  rbx, rax
    shl  rbx, 5                     ; 32 bytes per record
    add  rbx, rcx
    mov  [rbx], r10
    mov  [rbx + 8], r11
    mov  [rbx + 16], r12
    mov  qword [rbx + 24], %1
    inc  rax
%endmacro

; save callee-saved registers, build the masks from n (rdx)
%macro BEGIN 0
    push rbx
    push r12
    push r13
    push r14
    push r15
    mov  r15, rdx
    mov  r13, rdx                   ; n cycles
    mov  r8, 1
    mov  r14, 1
    mov  rbx, rcx
    mov  ecx, r15d
    shl  r8, cl
    dec  r8                         ; mask = 2^n - 1
    dec  ecx
    shl  r14, cl                    ; top bit = 2^(n-1)
    mov  rcx, rbx
    xor  eax, eax                   ; no records yet
    xor  r10d, r10d                 ; A = 0
    xor  r12d, r12d                 ; Q-1 / C = 0
%endmacro

%macro END 0
    pop  r15
    pop  r14
    pop  r13
    pop  r12
    pop  rbx
    RETURN
%endmacro

section .text
global vas_booth
global vas_add_shift
global vas_restoring_div
global vas_bin_to_bcd
global vas_bcd_to_bin

; -----------------------------------------------------------------------------
; Booth: look at Q0 Q-1   10 -> A = A - M   01 -> A = A + M
;        then arithmetic shift right A : Q : Q-1
; -----------------------------------------------------------------------------
vas_booth:
    ENTRY
    BEGIN
    mov  r9, rdi
    and  r9, r8                     ; M
    mov  r11, rsi
    and  r11, r8                    ; Q
    RECORD STEP_INIT
.cycle:
    mov  rdx, r11
    and  edx, 1
    shl  edx, 1
    or   rdx, r12                   ; rdx = Q0 Q-1
    cmp  edx, 2
    je   .sub
    cmp  edx, 1
    je   .add
    RECORD STEP_NOP
    jmp  .shift
.sub:
    sub  r10, r9
    and  r10, r8
    RECORD STEP_SUB
    jmp  .shift
.add:
    add  r10, r9
    and  r10, r8
    RECORD STEP_ADD
.shift:
    mov  r12, r11
    and  r12, 1                     ; Q-1 <- Q0
    shr  r11, 1
    test r10, 1
    jz   .keep_sign
    or   r11, r14                   ; A0 -> top bit of Q
.keep_sign:
    mov  rdx, r10
    and  rdx, r14                   ; arithmetic shift: the sign bit stays
    shr  r10, 1
    or   r10, rdx
    RECORD STEP_SHR
    dec  r13
    jnz  .cycle
    END

; -----------------------------------------------------------------------------
; add-and-shift: if Q0 = 1 then C,A = A + M ;  logical shift right C : A : Q
; -----------------------------------------------------------------------------
vas_add_shift:
    ENTRY
    BEGIN
    mov  r9, rdi
    and  r9, r8                     ; M
    mov  r11, rsi
    and  r11, r8                    ; Q
    lea  rdi, [r8 + 1]              ; 2^n = carry out of the n-bit adder
    RECORD STEP_INIT
.cycle:
    test r11, 1
    jz   .no_add
    add  r10, r9
    test r10, rdi
    setnz r12b                      ; C = carry out
    and  r10, r8
    RECORD STEP_ADD
    jmp  .shift
.no_add:
    RECORD STEP_NOP
.shift:
    shr  r11, 1
    test r10, 1
    jz   .a_shift
    or   r11, r14                   ; A0 -> top bit of Q
.a_shift:
    shr  r10, 1
    test r12, r12
    jz   .no_c
    or   r10, r14                   ; C -> top bit of A
.no_c:
    xor  r12d, r12d
    RECORD STEP_SHR
    dec  r13
    jnz  .cycle
    END

; -----------------------------------------------------------------------------
; restoring division:  shift left A : Q ;  A = A - M ;
;                      A < 0  ->  A = A + M (restore), Q0 = 0
;                      A >= 0 ->  Q0 = 1
; -----------------------------------------------------------------------------
vas_restoring_div:
    ENTRY
    BEGIN
    mov  r11, rdi
    and  r11, r8                    ; Q = dividend
    mov  r9, rsi
    and  r9, r8                     ; M = divisor
    jz   .done                      ; divide by zero: no steps
    lea  rdi, [r8*2 + 1]            ; A is n+1 bits: mask 2^(n+1) - 1
    lea  rsi, [r8 + 1]              ; sign bit of A = 2^n
    RECORD STEP_INIT
.cycle:
    shl  r10, 1
    test r11, r14
    jz   .q_shift
    or   r10, 1                     ; top bit of Q -> A0
.q_shift:
    and  r10, rdi
    shl  r11, 1
    and  r11, r8
    RECORD STEP_SHL
    sub  r10, r9
    and  r10, rdi
    RECORD STEP_SUB
    test r10, rsi
    jz   .positive
    add  r10, r9                    ; negative: restore
    and  r10, rdi
    RECORD STEP_RESTORE
    jmp  .next
.positive:
    or   r11, 1
    RECORD STEP_SETQ
.next:
    dec  r13
    jnz  .cycle
.done:
    END

; -----------------------------------------------------------------------------
; binary -> packed BCD: divide by 10, each remainder is the next nibble
; -----------------------------------------------------------------------------
vas_bin_to_bcd:
    ENTRY
    mov  rax, rdi
    xor  r8d, r8d                   ; packed result
    xor  ecx, ecx                   ; nibble position
    mov  r9d, 10
.digit:
    xor  edx, edx
    div  r9
    shl  rdx, cl
    or   r8, rdx
    add  ecx, 4
    test rax, rax
    jz   .done
    cmp  ecx, 64
    jb   .digit
.done:
    mov  rax, r8
    RETURN

; packed BCD -> binary: value = value * 10 + digit, high nibble first
vas_bcd_to_bin:
    ENTRY
    xor  eax, eax
    mov  ecx, 60
.digit:
    mov  rdx, rdi
    shr  rdx, cl
    and  edx, 0Fh
    imul rax, rax, 10
    add  rax, rdx
    sub  ecx, 4
    jns  .digit
    RETURN

SECTION_NOTE
