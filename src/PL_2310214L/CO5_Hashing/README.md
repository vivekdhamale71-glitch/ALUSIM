# 2310214L.CO5 — Apply suitable hashing techniques for solving given problems.

Variable names (compiler symbol table) and labels (assembler) are stored in a hash table whose collision strategy the user chooses: linear probing, quadratic probing or separate chaining. A generic chained HashMap provides O(1) lookups everywhere else.

## Where you see it in the simulator

- Data Lab screen, Hashing tab: inserts the program's variables and shows home slot, probes, collisions and load factor.
- Compiler screen: the symbol table panel.
- Compiler screen: the hash-method selector changes the table used by the compiler and the assembler.

## Files and functions

Paths are relative to `src/PL_2310214L/` unless they start with another top-level folder.

| File | Functions / classes | Role |
|---|---|---|
| HashTable.h / .cpp | `polyHash`, `nextPrime`, `makeHashTable` | base class, hash function, factory |
| OpenAddressing.h / .cpp | `insert`, `find`, `place`, `rehash`, `probeIndex` | linear and quadratic probing |
| Chaining.h / .cpp | `insert`, `find`, `rehash` | separate chaining |
| HashMap.h / .cpp | `put`, `find`, `erase`, `rehash` | generic map: opcodes, registers, memory, micro-code ROM, cache |

**Full explanation, worked example and viva questions:** [CO5_Explanation.pdf](CO5_Explanation.pdf)
