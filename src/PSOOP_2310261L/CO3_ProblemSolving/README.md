# 2310261L.CO3 — Apply critical thinking and programming skills for problem solving.

The central problem of the project: compile statements and simple C++ into x86-64 assembly. The compiler splits it into three stages (lexer, shunting-yard parser, code generator), each recorded so the screen can animate it.

## Where you see it in the simulator

- Compiler screen, English / C++ modes: token stream, shunting-yard animation, symbol table and the generated assembly.
- Parse & Chip → Parse tree: the expression tree built from the postfix.

## Files and functions

Paths are relative to `src/PSOOP_2310261L/` unless they start with another top-level folder.

| File | Functions / classes | Role |
|---|---|---|
| CO3_ProblemSolving/Compiler.h / .cpp | `Compiler::compile` | entry point, translation cache |
| ″ | `lex()` | characters → tokens (keywords by binary search) |
| ″ | `statement()`, `ifStatement()`, `whileStatement()`, `forStatement()` | statement parsing |
| ″ | `shunt()`, `genPostfix()`, `symbol()` | infix → postfix → instructions, symbol table |

**Full explanation, worked example and viva questions:** [CO3_Explanation.pdf](CO3_Explanation.pdf)
