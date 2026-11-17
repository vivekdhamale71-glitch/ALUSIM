# 2310261L.CO1 — Develop solutions for real world problems using Object Oriented Programming.

The simulator models a real computer as classes (CPU, ALU, Register64, Assembler, Compiler, ControlUnit, Pipeline) and solves a real problem for beginners: understanding code written in plain English. The EnglishTranslator turns sentences like “add 12 and 3” into program statements.

## Where you see it in the simulator

- Compiler screen, English mode: every word is coloured by its role (verb, number, variable, operator, target) and the matched rule is shown.

## Files and functions

Paths are relative to `src/PSOOP_2310261L/` unless they start with another top-level folder.

| File | Functions / classes | Role |
|---|---|---|
| CO1_RealWorldOOP/English.h / .cpp | `EnglishTranslator::translate` | English → statements |
| ″ | `statement()`, `expr()`, `term()`, `condition()`, `target()` | recursive-descent parser over words |
| core/* | `CPU`, `ALU`, `Register64`, `Assembler`, `ControlUnit`, `Pipeline` | hardware as objects |

**Full explanation, worked example and viva questions:** [CO1_Explanation.pdf](CO1_Explanation.pdf)
