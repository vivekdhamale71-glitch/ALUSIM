; =============================================================================
;  2310221L.CO4  Control-signal generation and pipeline timing
;  File : control_unit.asm   (NASM, linked into the simulator, called from C++)
;
;  Hardwired control unit
;  uint32 vas_decode(int form)
;      The decoder ROM: one row per instruction form. Returns the 11
;      control signals in bits 0..10 and the ALU operation in bits 16..23.
;        bit 0 PCWrite  1 IRWrite  2 MemRead  3 MemWrite  4 RegWrite
;            5 ALUSrc   6 MemToReg 7 Branch   8 Jump      9 FlagWrite
;           10 StackOp
;      Unknown form -> 0 (no signals: behaves like NOP).
;  int vas_decode_rows(void)   number of rows in the ROM
;
;  5-stage pipeline (IF ID EX MEM WB), stage numbers are clock cycles
;  int vas_operand_ready(int mem, int wb, int forwarding, int load)
;      Earliest EX cycle for an instruction that reads a register written by
;      a producer whose MEM / WB stages start in cycles mem / wb:
;        no forwarding : wb + 1       (written in WB, read in the next ID)
;        forwarding    : mem          (ALU result forwarded from EX/MEM)
;                        wb           (load data forwarded from MEM/WB)
;  int vas_schedule(const int prev[5], int fetch, int ready, int out[5])
;      Start cycles of IF ID EX MEM WB for the next instruction. prev is the
;      previous instruction (or NULL), ready the earliest EX allowed by data
;      hazards. An instruction cannot enter a stage before the one ahead of
;      it has left it. Returns the number of stall cycles.
; =============================================================================

%include "COA_2310221L/abi.inc"

%define PCW   (1 << 0)
%define IRW   (1 << 1)
%define MRD   (1 << 2)
%define MWR   (1 << 3)
%define RGW   (1 << 4)
%define SRC   (1 << 5)
%define M2R   (1 << 6)
%define BR    (1 << 7)
%define JMP   (1 << 8)
%define FLW   (1 << 9)
%define STK   (1 << 10)
%define ALU(op) ((op) << 16)

; AluOp numbers (same order as the C++ enum)
%define A_NONE 0
%define A_PASS 1
%define A_ADD  2
%define A_SUB  3
%define A_DIV  5
%define A_INC  13

section .rodata
align 4
decoder_rom:                                            ; form
    dd RGW | FLW                     | ALU(A_ADD)      ; ALU  reg, reg
    dd RGW | SRC | FLW               | ALU(A_ADD)      ; ALU  reg, imm
    dd MRD | RGW | SRC | FLW         | ALU(A_ADD)      ; ALU  reg, [mem]
    dd FLW                           | ALU(A_SUB)      ; CMP / TEST
    dd RGW | FLW                     | ALU(A_INC)      ; INC DEC NEG NOT
    dd RGW                           | ALU(A_PASS)     ; MOV  reg, reg
    dd RGW | SRC                     | ALU(A_PASS)     ; MOV  reg, imm
    dd MRD | RGW | SRC | M2R         | ALU(A_ADD)      ; MOV  reg, [mem]
    dd MWR | SRC                     | ALU(A_ADD)      ; MOV  [mem], src
    dd BR                            | ALU(A_NONE)     ; Jcc
    dd PCW | JMP                     | ALU(A_NONE)     ; JMP
    dd MWR | RGW | SRC | STK         | ALU(A_SUB)      ; PUSH
    dd MRD | RGW | SRC | M2R | STK   | ALU(A_ADD)      ; POP
    dd PCW | MWR | RGW | SRC | JMP | STK | ALU(A_SUB)  ; CALL
    dd PCW | MRD | RGW | SRC | JMP | STK | ALU(A_ADD)  ; RET
    dd RGW                           | ALU(A_DIV)      ; IDIV
    dd RGW                           | ALU(A_PASS)     ; CQO
    dd 0                                               ; HLT
    dd 0                                               ; NOP
ROM_ROWS equ ($ - decoder_rom) / 4

section .text
global vas_decode
global vas_decode_rows
global vas_operand_ready
global vas_schedule

; -----------------------------------------------------------------------------
vas_decode:
    ENTRY
    xor  eax, eax
    cmp  edi, ROM_ROWS
    jae  .out                       ; also catches negative forms (unsigned)
    lea  rax, [rel decoder_rom]
    mov  eax, [rax + rdi*4]
.out:
    RETURN

vas_decode_rows:
    ENTRY
    mov  eax, ROM_ROWS
    RETURN

; -----------------------------------------------------------------------------
; edi = mem, esi = wb, edx = forwarding, ecx = load
vas_operand_ready:
    ENTRY
    test edx, edx
    jnz  .forwarding
    lea  eax, [rsi + 1]             ; wait for write-back
    RETURN
.forwarding:
    mov  eax, edi                   ; ALU result: after EX
    test ecx, ecx
    cmovnz eax, esi                 ; load: data exists only after MEM
    RETURN

; -----------------------------------------------------------------------------
; rdi = prev (may be NULL), esi = fetch cycle, edx = ready, rcx = out
%define S_IF  0
%define S_ID  4
%define S_EX  8
%define S_MEM 12
%define S_WB  16

vas_schedule:
    ENTRY
    xor  r8d, r8d                   ; prev EX  start (0 = no previous)
    xor  r9d, r9d                   ; prev MEM start
    xor  r10d, r10d                 ; prev WB  start
    test rdi, rdi
    jz   .stages
    mov  r8d, [rdi + S_EX]
    mov  r9d, [rdi + S_MEM]
    mov  r10d, [rdi + S_WB]
.stages:
    mov  [rcx + S_IF], esi
    lea  eax, [rsi + 1]             ; ID = max(IF + 1, prev EX)
    cmp  eax, r8d
    cmovl eax, r8d
    mov  [rcx + S_ID], eax
    mov  r11d, eax                  ; keep ID
    inc  eax                        ; EX = max(ID + 1, prev MEM, ready)
    cmp  eax, r9d
    cmovl eax, r9d
    cmp  eax, edx
    cmovl eax, edx
    mov  [rcx + S_EX], eax
    mov  edx, eax                   ; keep EX
    inc  eax                        ; MEM = max(EX + 1, prev WB)
    cmp  eax, r10d
    cmovl eax, r10d
    mov  [rcx + S_MEM], eax
    lea  r8d, [rax + 1]             ; WB = MEM + 1
    mov  [rcx + S_WB], r8d
    ; stalls = (ID - IF - 1) + (EX - ID - 1) + (MEM - EX - 1) = MEM - IF - 3
    sub  eax, esi
    sub  eax, 3
    RETURN

SECTION_NOTE
