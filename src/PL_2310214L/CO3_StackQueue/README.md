# 2310214L.CO3 — Build scenario based applications using stack and queue  [L3]

**Files:** `Stack.h` (dynamic-array stack), `Queue.h` (circular queue, optional overwrite mode).

**Scenarios:**
- **Shunting-yard** (infix → postfix): operator **stack** + output **queue** — `Compiler.cpp`
- code generation keeps a **value stack** of registers / constants / memory operands
- the **parse tree** is built with a stack (Parse tree screen)
- the CPU executes **PUSH / POP / CALL / RET** on its stack
- the waveform is a rolling **circular queue** of control-signal samples (`app/Sim.h`)

**See it in the app:** Compiler → Parser card · Parse & Chip → stack column · sample "ASM: loop + CALL / RET".
