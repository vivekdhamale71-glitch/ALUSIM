# 2310221L.CO4 — Demonstrate the different ways to generate control signals and their organization techniques and instruction pipelining of computer system.

The hardwired control unit's decoder ROM is in assembly: one 32-bit word per instruction form holding the 11 control signals and the ALU operation. The 5-stage pipeline's timing rules (operand readiness with and without forwarding, and stage scheduling with stalls) are also assembly functions.

## Where you see it in the simulator

- Datapath screen: Hardwired / Microprogrammed switch, the decoder table, the 11-signal vector each cycle and the waveform.
- Pipeline screen: space-time diagram, stalls, forwarding arrows, CPI and speed-up.

## Files and functions

Paths are relative to `src/COA_2310221L/` unless they start with another top-level folder.

| File | Functions / classes | Role |
|---|---|---|
| CO4_ControlUnitPipeline/control_unit.asm | `vas_decode(form)`, `vas_decode_rows()` | hardwired decoder ROM |
| ″ | `vas_operand_ready(mem, wb, fwd, load)` | earliest EX cycle for a dependent instruction |
| ″ | `vas_schedule(prev, fetch, ready, out)` | IF ID EX MEM WB start cycles + stall count |
| core/ControlUnit.cpp | `MicroprogrammedControlUnit` | control store, micro-PC, dispatch ROM |
| core/Pipeline.cpp | `PipelineSimulator::run` | builds the diagram from the execution trace |

**Full explanation, worked example and viva questions:** [CO4_Explanation.pdf](CO4_Explanation.pdf)
