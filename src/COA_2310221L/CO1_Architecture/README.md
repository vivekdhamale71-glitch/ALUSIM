# 2310221L.CO1 — Explicate the architecture and instruction set of the 80386 microprocessor.

The simulator's ALU is written in x86-64 assembly and runs on the real processor's ALU: the simulated flags are loaded into RFLAGS, the real ADD / SUB / IMUL / AND / SHL ... runs, and the processor's own CF ZF SF OF are read back.

## Where you see it in the simulator

- Every arithmetic or logic instruction of every program goes through `vas_alu`.
- Registers panel: RAX with its EAX / AX / AH / AL views (8086 → 80386 → x86-64).
- The CF ZF SF OF lamps under the registers show the flags returned by the real CPU.

## Files and functions

Paths are relative to `src/COA_2310221L/` unless they start with another top-level folder.

| File | Functions / classes | Role |
|---|---|---|
| CO1_Architecture/alu.asm | `vas_alu(op, a, b, &rflags)` | the ALU: 14 operations + flags |
| abi.inc | `ENTRY` / `RETURN` | System V (Linux) and Windows x64 calling conventions |
| coa_asm.h | `extern "C"` declarations | how C++ calls the assembly |
| core/Register64.h | `ALU::compute`, sub-register views | C++ side: calls vas_alu |

**Full explanation, worked example and viva questions:** [CO1_Explanation.pdf](CO1_Explanation.pdf)
