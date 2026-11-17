# 2310261L.CO4 — Logic building for real-life problems  [L3]

**Files:** `Patterns.h / .cpp`
- `Debugger` — **friend** class of `CPU`, reads private state for the trace export (Ctrl+E)
- `Profiler` — **Observer** of the CPU, counts instructions (feeds the sorting demo)
- `Clock` — **Singleton** cycle counter

Elsewhere: **Factory** (`InstructionFactory`), **Strategy** (control units, hash methods), **Command / undo**
(Step Back history in `app/Sim.cpp`), static members, file handling (save / load / trace export).
