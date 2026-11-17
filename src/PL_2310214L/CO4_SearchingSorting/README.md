# 2310214L.CO4 — Solve given problems using searching and sorting techniques.

The assembler sorts its opcode table once with quick sort and then finds every mnemonic with binary search; the lexer finds keywords the same way. The Data Lab animates bubble, insertion and quick sort on the profiler's instruction counts, and linear, binary and Fibonacci search on data memory.

## Where you see it in the simulator

- Data Lab screen, Sorting tab: compares and swaps of the three sorts, with counts.
- Data Lab screen, Searching tab: each probe of the three searches on memory.
- Every assembled instruction: its mnemonic is found by binary search.

## Files and functions

Paths are relative to `src/PL_2310214L/` unless they start with another top-level folder.

| File | Functions / classes | Role |
|---|---|---|
| Sorting.h / Sorting.cpp | `BubbleSort`, `InsertionSort`, `QuickSort` | Data Lab sorts (record every step) |
| Searching.h / Searching.cpp | `LinearSearch`, `BinarySearch`, `FibonacciSearch` | Data Lab searches |
| TableSort.h / .cpp | `quickSort` | sorts the opcode table at start-up |
| TableSearch.h / .cpp | `binarySearch` | mnemonic and keyword lookup |

**Full explanation, worked example and viva questions:** [CO4_Explanation.pdf](CO4_Explanation.pdf)
