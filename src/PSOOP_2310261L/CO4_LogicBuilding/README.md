# 2310261L.CO4 — Utilize logic building traits efficiently for solving real life problems.

Design patterns solve recurring problems in the simulator: a Factory creates instructions from mnemonics, an Observer profiles execution, a Singleton holds the clock, Strategies make the control unit, hash method and sort interchangeable, and Command / undo powers Step Back. A friend class and a static member complete the set.

## Where you see it in the simulator

- Top bar: “cycle N” is the Singleton clock; “executed N” is the static member Instruction::executedCount.
- Data Lab sorts the Profiler's counts (Observer).
- Ctrl+E exports the trace written by the friend class Debugger.
- Step Back undoes instructions (Command / undo).

## Files and functions

Paths are relative to `src/PSOOP_2310261L/` unless they start with another top-level folder.

| File | Functions / classes | Role |
|---|---|---|
| CO4_LogicBuilding/Clock.h / .cpp | `Clock::instance()` | Singleton |
| CO4_LogicBuilding/Profiler.h / .cpp | `CPUObserver`, `Profiler::onStep` | Observer |
| CO4_LogicBuilding/Debugger.h / .cpp | `dumpRegisters`, `describe` | friend class of CPU |
| core/Instruction.cpp | `InstructionFactory`, `executedCount` | Factory, static member |
| app/Sim.cpp | `stepBack()`, `exportTrace()`, `saveSource()` | Command / undo, file handling |

**Full explanation, worked example and viva questions:** [CO4_Explanation.pdf](CO4_Explanation.pdf)
