# 2310221L.CO4 — Control signals, organisation and instruction pipelining  [L3]

**Files:**
- `ControlUnit.h / .cpp` — **Hardwired** control unit (decoder table) and **Microprogrammed** control unit (41-word control store, micro-PC, dispatch ROM); both produce the 11-signal control vector for every cycle
- `Pipeline.h / .cpp` — 5-stage pipeline (IF ID EX MEM WB) from the real execution trace: data / control / structural hazards, forwarding, stalls, flushes, CPI, speed-up

**See it in the app:** Datapath (toggle Hardwired / Microprogrammed, waveform) · Pipeline screen (3D conveyor + space-time diagram).
