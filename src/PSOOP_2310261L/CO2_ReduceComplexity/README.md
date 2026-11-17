# 2310261L.CO2 — Choose a programming solution that reduces complexity  [L3]

**File:** `Errors.h` — one **exception hierarchy** (`SimulatorError` → `SyntaxError`, `InvalidOpcodeError`,
`DivideByZeroError`, `MemoryAccessError`, `StackOverflowError`, `RuntimeLimitError`) shared by every layer, so errors travel
to the UI through `try / catch` instead of error codes threaded through every function.

Other complexity-reducing choices in the codebase: the abstract `Instruction` class (the CPU only calls `execute()`),
the `ControlUnit` / `HashTable` / `Sorter` / `Searcher` / `View` interfaces, and the template containers in `PL_2310214L`.
