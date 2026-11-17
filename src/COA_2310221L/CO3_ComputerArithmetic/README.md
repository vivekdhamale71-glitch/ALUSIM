# 2310221L.CO3 — Illustrate processor level details about arithmetic operations using computer arithmetic algorithms.

Booth's multiplication, add-and-shift multiplication, restoring division and binary / BCD conversion are written in assembly on n-bit registers. Each records every step (A, Q, Q-1 or C, operation) into an array that the Arithmetic screen animates bit by bit.

## Where you see it in the simulator

- Arithmetic screen: choose Booth's / Add & Shift / Restoring Division / Hex ↔ BCD; the 3D register cubes and the step table come from these functions.
- “From last IMUL / IDIV” takes the operands from the program that just ran.

## Files and functions

Paths are relative to `src/COA_2310221L/` unless they start with another top-level folder.

| File | Functions / classes | Role |
|---|---|---|
| CO3_ComputerArithmetic/arithmetic.asm | `vas_booth` | signed multiplication |
| ″ | `vas_add_shift` | unsigned multiplication |
| ″ | `vas_restoring_div` | unsigned division |
| ″ | `vas_bin_to_bcd`, `vas_bcd_to_bin` | number systems |
| core/Arithmetic.cpp | `boothMultiply` ... | turns the step records into text for the screen |

**Full explanation, worked example and viva questions:** [CO3_Explanation.pdf](CO3_Explanation.pdf)
