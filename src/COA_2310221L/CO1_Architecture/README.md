# 2310221L.CO1 — Architecture and instruction set of the 80386  [L2]

**Files:**
- `Register64.h` — 64-bit register with the 80386/8086 views **EAX / AX / AH / AL**, the RFLAGS bits (CF ZF SF OF) and the **ALU** (flags computed exactly like x86)
- `CPU.h / CPU.cpp` — the processor: register file, RIP, flags, sparse memory, stack, fetch–execute loop

**See it in the app:** Registers panel + sub-register bar (RAX ⊃ EAX ⊃ AX ⊃ AH/AL) · flag LEDs · Datapath screen.
