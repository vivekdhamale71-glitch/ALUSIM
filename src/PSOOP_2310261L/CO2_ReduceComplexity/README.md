# 2310261L.CO2 — Choose an appropriate programming solution to reduce complexity.

Complexity is reduced with abstraction and inheritance: the CPU only calls execute() on an abstract Instruction; control units, hash tables, sorters and screens are abstract base classes with interchangeable subclasses; containers are templates; and every error is one typed exception hierarchy caught in one place.

## Where you see it in the simulator

- Any error (syntax, divide by zero, stack overflow...) shows as one consistent error toast with the line number.

## Files and functions

Paths are relative to `src/PSOOP_2310261L/` unless they start with another top-level folder.

| File | Functions / classes | Role |
|---|---|---|
| CO2_ReduceComplexity/Errors.h / .cpp | `SimulatorError` and 6 subclasses | exception hierarchy |
| core/Instruction.h | `Instruction` → DataTransfer, Arithmetic, Logic, ControlFlow, Stack, System | polymorphic instructions |
| core/ControlUnit.h | `ControlUnit` → Hardwired, Microprogrammed | abstract control unit |
| PL CO4 / CO5 | `Sorter`, `Searcher`, `HashTable`; templates `Stack`, `HashMap` | abstraction and generic code |

**Full explanation, worked example and viva questions:** [CO2_Explanation.pdf](CO2_Explanation.pdf)
