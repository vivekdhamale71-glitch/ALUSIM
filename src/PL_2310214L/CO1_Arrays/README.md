# 2310214L.CO1 — Develop applications using arrays.

The CPU's register file is an array: 16 registers addressed by number, so reading or writing RAX, RBX ... R15 is one O(1) index operation.

## Where you see it in the simulator

- Registers panel (right side of every screen): the 16 rows are the 16 array elements.
- Every instruction that reads or writes a register goes through this array.

## Files and functions

Paths are relative to `src/PL_2310214L/` unless they start with another top-level folder.

| File | Functions / classes | Role |
|---|---|---|
| CO1_Arrays/FixedArray.h | class `FixedArray` | the array type: size fixed at compile time |
| CO1_Arrays/FixedArray.cpp | `at()`, `fill()`, constructor | bounds-checked access and traversal |
| core/CPU.h | `FixedArray regs_` | the register file of the simulated CPU |

**Full explanation, worked example and viva questions:** [CO1_Explanation.pdf](CO1_Explanation.pdf)
