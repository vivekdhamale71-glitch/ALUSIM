# 2310214L.CO1 — Develop applications using arrays  [L3]

**File:** `FixedArray.h` — `FixedArray<T, N>`: fixed-size array with O(1) indexed access and bounds-checked `at()`.

**Used for:** the CPU register file — `FixedArray<Register64, 16>` in `COA_2310221L/CO1_Architecture/CPU.h`.
Registers are addressed by number (RAX = 0 … R15 = 15), so an array is the natural structure.
Arrays also hold the sorted opcode table, the open-addressing hash-table slots and the waveform columns.

**See it in the app:** the Registers panel on the right of every screen.
