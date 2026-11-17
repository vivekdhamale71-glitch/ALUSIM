# 2310214L.CO2 — Choose suitable type of linked list for various applications.

Three different linked lists, each picked for its shape: singly for the lexer's token stream, doubly for the Step Back history, circular for the FETCH-DECODE-EXECUTE-MEMORY-WRITEBACK cycle.

## Where you see it in the simulator

- Compiler screen: the token stream is a singly linked list.
- Step Back (left arrow / F9): walks back through the doubly linked history.
- Datapath screen: the phase timeline IF ID EX MEM WB is drawn by walking the circular list, and the Control Signals card shows “EXECUTE → MEMORY”, the next node of the ring.

## Files and functions

Paths are relative to `src/PL_2310214L/` unless they start with another top-level folder.

| File | Functions / classes | Role |
|---|---|---|
| SinglyLinkedList.h / .cpp | `pushBack`, `clear`, iterator | token stream (PSOOP Compiler.cpp) |
| DoublyLinkedList.h / .cpp | `pushBack`, `truncateAfter` | execution history (app/Sim.cpp) |
| CircularLinkedList.h / .cpp | `insert`, `find`, `clear` | instruction-cycle ring (app/Sim.cpp, DatapathView.cpp) |

**Full explanation, worked example and viva questions:** [CO2_Explanation.pdf](CO2_Explanation.pdf)
