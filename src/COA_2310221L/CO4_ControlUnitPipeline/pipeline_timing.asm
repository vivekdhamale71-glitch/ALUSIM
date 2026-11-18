; The cycle in which every pipeline stage starts, with stalls for data hazards and forwarding.

default rel

; offsets of the five stage start cycles in an array of five 32-bit numbers
S_IF  equ 0
S_ID  equ 4
S_EX  equ 8
S_MEM equ 12
S_WB  equ 16

section .text
global operand_ready, stage_schedule

; operand_ready(edi = MEM cycle of the producer, esi = WB cycle of the producer,
;               edx = 1 if forwarding is on, ecx = 1 if the producer is a load) -> eax : the earliest EX cycle of the consumer
;   without forwarding the value is written back in WB, so the consumer may execute one cycle later;
;   with forwarding an ALU result is passed on after its EX stage (the MEM cycle of the producer),
;   but a loaded value exists only after the MEM stage (the WB cycle), which costs one bubble
operand_ready:
    test edx, edx
    jnz .forwarding
    lea eax, [rsi + 1]              ; wait for the write-back
    ret
.forwarding:
    mov eax, edi                    ; an ALU result is ready after EX
    test ecx, ecx
    cmovnz eax, esi                 ; a loaded value is ready only after MEM
    ret

; stage_schedule(rdi = the previous instruction's five start cycles or 0, esi = fetch cycle,
;                edx = earliest EX cycle allowed by data hazards, rcx = where to store the five start cycles)
;   -> eax : stall cycles
;   an instruction cannot enter a stage before the instruction ahead of it has left it:
;     ID = max(IF + 1, EX of the previous)           EX = max(ID + 1, MEM of the previous, edx)
;     MEM = max(EX + 1, WB of the previous)          WB = MEM + 1
stage_schedule:
    xor r8d, r8d                    ; EX of the previous instruction (0 if there is none)
    xor r9d, r9d                    ; MEM of the previous
    xor r10d, r10d                  ; WB of the previous
    test rdi, rdi
    jz .stages
    mov r8d, [rdi + S_EX]
    mov r9d, [rdi + S_MEM]
    mov r10d, [rdi + S_WB]
.stages:
    mov [rcx + S_IF], esi
    lea eax, [rsi + 1]              ; ID
    cmp eax, r8d
    cmovl eax, r8d
    mov [rcx + S_ID], eax
    inc eax                         ; EX
    cmp eax, r9d
    cmovl eax, r9d
    cmp eax, edx
    cmovl eax, edx
    mov [rcx + S_EX], eax
    inc eax                         ; MEM
    cmp eax, r10d
    cmovl eax, r10d
    mov [rcx + S_MEM], eax
    lea r8d, [rax + 1]              ; WB
    mov [rcx + S_WB], r8d
    sub eax, esi                    ; stalls = MEM - IF - 3 (an instruction without stalls has MEM = IF + 3)
    sub eax, 3
    ret
