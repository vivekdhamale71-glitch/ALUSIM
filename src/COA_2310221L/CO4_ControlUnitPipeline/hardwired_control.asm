; A decoder built from AND and OR logic that turns an opcode into the control signals.

default rel

; control signals (bit positions in the control word)
SIG_REGDST    equ 0
SIG_ALUSRC    equ 1
SIG_MEMTOREG  equ 2
SIG_REGWRITE  equ 3
SIG_MEMREAD   equ 4
SIG_MEMWRITE  equ 5
SIG_BRANCH    equ 6
SIG_JUMP      equ 7
SIG_ALUOP1    equ 8
SIG_ALUOP0    equ 9

section .text
global hardwired_decode, signal_active

; hardwired_decode(rdi = opcode of 3 bits) -> rax : the control word (one bit per signal)
;   opcodes: 000 R-type, 001 LW, 010 SW, 011 BEQ, 100 JMP, 101 ADDI
;   exactly like the gates of a decoder: first an AND per opcode, then an OR per signal
hardwired_decode:
    push rbx
    push r12
    push r13
    mov eax, edi
    and eax, 1                      ; o0
    mov ecx, edi
    shr ecx, 1
    and ecx, 1                      ; o1
    mov edx, edi
    shr edx, 2
    and edx, 1                      ; o2
    mov r8d, eax
    xor r8d, 1                      ; not o0
    mov r9d, ecx
    xor r9d, 1                      ; not o1
    mov r10d, edx
    xor r10d, 1                     ; not o2

    mov r11d, r10d                  ; R-type = not o2, not o1, not o0
    and r11d, r9d
    and r11d, r8d
    mov ebx, r10d                   ; LW = not o2, not o1, o0
    and ebx, r9d
    and ebx, eax
    mov r12d, r10d                  ; SW = not o2, o1, not o0
    and r12d, ecx
    and r12d, r8d
    mov r13d, r10d                  ; BEQ = not o2, o1, o0
    and r13d, ecx
    and r13d, eax
    mov esi, edx                    ; JMP = o2, not o1, not o0
    and esi, r9d
    and esi, r8d
    mov edi, edx                    ; ADDI = o2, not o1, o0
    and edi, r9d
    and edi, eax

    xor eax, eax                    ; now one OR gate per signal
    mov ecx, r11d                   ; RegDst = R-type
    shl ecx, SIG_REGDST
    or eax, ecx
    mov ecx, ebx                    ; ALUSrc = LW or SW or ADDI
    or ecx, r12d
    or ecx, edi
    shl ecx, SIG_ALUSRC
    or eax, ecx
    mov ecx, ebx                    ; MemToReg = LW
    shl ecx, SIG_MEMTOREG
    or eax, ecx
    mov ecx, r11d                   ; RegWrite = R-type or LW or ADDI
    or ecx, ebx
    or ecx, edi
    shl ecx, SIG_REGWRITE
    or eax, ecx
    mov ecx, ebx                    ; MemRead = LW
    shl ecx, SIG_MEMREAD
    or eax, ecx
    mov ecx, r12d                   ; MemWrite = SW
    shl ecx, SIG_MEMWRITE
    or eax, ecx
    mov ecx, r13d                   ; Branch = BEQ
    shl ecx, SIG_BRANCH
    or eax, ecx
    mov ecx, esi                    ; Jump = JMP
    shl ecx, SIG_JUMP
    or eax, ecx
    mov ecx, r11d                   ; ALUOp1 = R-type
    shl ecx, SIG_ALUOP1
    or eax, ecx
    mov ecx, r13d                   ; ALUOp0 = BEQ
    shl ecx, SIG_ALUOP0
    or eax, ecx
    pop r13
    pop r12
    pop rbx
    ret

; signal_active(rdi = control word, esi = signal number) -> eax : 1 if that signal is switched on
signal_active:
    mov rax, rdi
    mov ecx, esi
    shr rax, cl
    and eax, 1
    ret
