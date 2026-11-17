# 2310214L.CO2 — Choose suitable type of linked list for various applications  [L3]

**File:** `LinkedList.h` — three list types, each chosen for the job it does:

| List | Used for | Why this type |
|---|---|---|
| `SinglyLinkedList<T>` | the lexer's token stream (`PSOOP_2310261L/CO3_ProblemSolving/Compiler.cpp`) | built once, read front to back |
| `DoublyLinkedList<T>` | execution history of CPU snapshots (`app/Sim.h`) | **Step Back** needs to walk backwards |
| `CircularLinkedList<T>` | the FETCH → DECODE → EXECUTE → MEMORY → WB cycle (`app/Sim.h`) | the instruction cycle repeats forever |

Separate chaining in `CO5_Hashing/HashTable.h` is also a linked list per bucket.

**See it in the app:** Compiler → token stream · the Step Back button · Chaining in the symbol table.
