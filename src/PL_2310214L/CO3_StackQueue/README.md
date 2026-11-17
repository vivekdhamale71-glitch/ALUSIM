# 2310214L.CO3 — Build scenario based applications using stack and queue.

The compiler turns infix expressions into postfix with the shunting-yard algorithm, which needs an operator stack and an output queue; code generation then uses a value stack and a stack of free registers. The waveform is a circular queue.

## Where you see it in the simulator

- Compiler screen: the shunting-yard animation shows the operator stack and the output queue.
- Datapath screen: the control-signal waveform keeps the last 56 cycles in a circular queue.

## Files and functions

Paths are relative to `src/PL_2310214L/` unless they start with another top-level folder.

| File | Functions / classes | Role |
|---|---|---|
| Stack.h / Stack.cpp | `push`, `pop`, `top`, `grow` | dynamic-array stack (LIFO) |
| Queue.h / Queue.cpp | `enqueue`, `dequeue`, `front`, `grow` | circular queue (FIFO), optional overwrite |
| PSOOP .../Compiler.cpp | `shunt()`, `genPostfix()` | the scenario: expression compilation |
| app/Sim.h | `CircularQueue wave{56, true}` | rolling waveform buffer |

**Full explanation, worked example and viva questions:** [CO3_Explanation.pdf](CO3_Explanation.pdf)
