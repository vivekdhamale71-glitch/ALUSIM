; =============================================================================
;  2310221L.CO1  The simulator's ALU, running on the real x86-64 ALU
;  File : alu.asm   (NASM, linked into the simulator, called from C++)
;
;  uint64_t vas_alu(int op, uint64_t a, uint64_t b, uint64_t* flags)
;     op     AluOp:  0 None 1 Pass 2 Add 3 Sub 4 Mul 5 Div 6 And 7 Or 8 Xor
;                    9 Not 10 Neg 11 Shl 12 Shr 13 Inc 14 Dec
;     flags  in : current RFLAGS of the simulated CPU (CF ZF SF OF)
;            out: RFLAGS after the operation
;     returns the result
;
;  How it works: the simulated flags are loaded into the real RFLAGS with
;  POPFQ, the matching x86 instruction runs, and PUSHFQ reads the flags
;  back. So CF / ZF / SF / OF, and the flags an instruction leaves
;  unchanged (INC keeps CF, NOT keeps all), come from the processor itself.
; =============================================================================

%include "COA_2310221L/abi.inc"

%define STATUS_MASK  0x8C1          ; OF(11) SF(7) ZF(6) CF(0)
%define OF_BIT       0x800

section .rodata
align 4
alu_table:                          ; offsets from the table (position independent)
    dd op_none - alu_table, op_pass - alu_table, op_add - alu_table, op_sub - alu_table
    dd op_mul - alu_table,  op_div - alu_table,  op_and - alu_table, op_or - alu_table
    dd op_xor - alu_table,  op_not - alu_table,  op_neg - alu_table, op_shl - alu_table
    dd op_shr - alu_table,  op_inc - alu_table,  op_dec - alu_table
ALU_OPS equ ($ - alu_table) / 4

section .text
global vas_alu

vas_alu:
    ENTRY
    mov  r8, rcx                    ; r8  = flags pointer
    mov  r9, [r8]
    and  r9, STATUS_MASK            ; only the status flags reach the CPU
    or   r9, 2                      ; bit 1 is always 1

    cmp  edi, ALU_OPS
    jae  op_none
    cmp  edi, 5
    je   op_div                     ; IDIV needs its own checks (no #DE here)

    lea  r10, [rel alu_table]
    movsxd r11, dword [r10 + rdi*4]
    add  r10, r11                   ; choose the operation first ...
    mov  rcx, rdx                   ; shift count in CL
    mov  rax, rsi                   ; rax = a (destination operand)
    push r9
    popfq                           ; ... then load the simulated flags
    jmp  r10                        ; JMP does not touch RFLAGS

op_none:
    mov  rax, rsi
    jmp  done_keep
op_pass:
    mov  rax, rdx                   ; MOV: result = b, flags unchanged
    jmp  done
op_add:
    add  rax, rdx
    jmp  done
op_sub:
    sub  rax, rdx                   ; also CMP: result discarded by the caller
    jmp  done
op_and:
    and  rax, rdx
    jmp  done
op_or:
    or   rax, rdx
    jmp  done
op_xor:
    xor  rax, rdx
    jmp  done
op_not:
    not  rax                        ; NOT changes no flags
    jmp  done
op_neg:
    neg  rax
    jmp  done
op_inc:
    inc  rax                        ; INC / DEC keep CF
    jmp  done
op_dec:
    dec  rax
    jmp  done

op_shl:
    shl  rax, cl                    ; count is masked to 6 bits by the CPU
    jmp  shift_done
op_shr:
    shr  rax, cl
shift_done:
    pushfq
    pop  r10
    and  ecx, 63
    cmp  ecx, 1
    je   store_r10                  ; OF is defined only for a 1-bit shift
    and  r10, ~OF_BIT
    mov  r11, r9
    and  r11, OF_BIT
    or   r10, r11                   ; otherwise keep the old OF
    jmp  store_r10

op_mul:
    imul rax, rdx                   ; CF = OF = result did not fit in 64 bits
    pushfq
    pop  r10
    and  r10, 0x801                 ; keep CF, OF from IMUL
    test rax, rax                   ; ZF, SF of the 64-bit product
    pushfq
    pop  r11
    and  r11, 0xC0
    or   r10, r11
    jmp  store_r10

; signed quotient a / b. The CPU already threw #DE for b = 0; INT64_MIN / -1
; would trap on real hardware, so it returns a unchanged. Flags unchanged.
op_div:
    mov  rax, rsi
    test rdx, rdx
    jz   done_keep
    mov  r10, rdx
    cmp  r10, -1
    jne  .divide
    mov  r11, 0x8000000000000000
    cmp  rax, r11
    je   done_keep
.divide:
    cqo                             ; sign-extend RAX into RDX:RAX
    idiv r10
    jmp  done_keep

done:
    pushfq
    pop  r10
store_r10:
    and  r10, STATUS_MASK
    or   r10, 2
    mov  [r8], r10
    RETURN
done_keep:
    mov  [r8], r9
    RETURN

SECTION_NOTE
