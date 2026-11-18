; =============================================================================
;  abi.inc - shared by the COA .asm files
;  Every function is written for the System V AMD64 convention (Linux):
;      arguments rdi, rsi, rdx, rcx      result rax
;      rbx rbp r12-r15 must be preserved
;  On Windows x64 (nasm -f win64) the arguments arrive in rcx, rdx, r8, r9
;  and rdi / rsi are callee-saved, so ENTRY / RETURN translate.
;  Only the first four arguments are used, all in registers.
; =============================================================================
%ifidn __OUTPUT_FORMAT__, win64
    %macro ENTRY 0
        push rdi
        push rsi
        mov  rdi, rcx
        mov  rsi, rdx
        mov  rdx, r8
        mov  rcx, r9
    %endmacro
    %macro RETURN 0
        pop  rsi
        pop  rdi
        ret
    %endmacro
    %macro SECTION_NOTE 0
    %endmacro
%else
    %macro ENTRY 0
    %endmacro
    %macro RETURN 0
        ret
    %endmacro
    ; mark the stack non-executable so the linker does not warn
    %macro SECTION_NOTE 0
        section .note.GNU-stack noalloc noexec nowrite progbits
    %endmacro
%endif

default rel
