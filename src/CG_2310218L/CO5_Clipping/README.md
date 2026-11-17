# 2310218L.CO5 — Implement the suitable clipping algorithms.

When the datapath is zoomed or panned, every wire is clipped to the diagram's viewport with Cohen-Sutherland; the scrolling waveform traces are clipped to the plot window with Liang-Barsky.

## Where you see it in the simulator

- Datapath screen: zoom in, wires stop exactly at the card's edge.
- Control-signal waveform: traces are cut at the plot's left and right edges as cycles scroll by.

## Files and functions

Paths are relative to `src/CG_2310218L/` unless they start with another top-level folder.

| File | Functions / classes | Role |
|---|---|---|
| CO5_Clipping/Clip.h / Clip.cpp | `cohenSutherland(a, b, rect)` | region codes, trivial accept/reject |
| ″ | `liangBarsky(a, b, rect)` | parametric clipping |

**Full explanation, worked example and viva questions:** [CO5_Explanation.pdf](CO5_Explanation.pdf)
