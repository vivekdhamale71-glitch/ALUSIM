# 2310218L.CO3 — Develop 2D and 3D objects using various algorithms.

Bresenham's line algorithm turns active wires into flowing signal dots, the midpoint circle algorithm draws the flag LEDs and wire junctions, the scan-line algorithm fills the ALU and multiplexer symbols, and extrusion turns the same 2D outlines into the 3D chip.

## Where you see it in the simulator

- Datapath screen: dots running along active wires (Bresenham), junction rings (midpoint), filled ALU / MUX shapes (scan-line).
- Flag LEDs under the registers (midpoint circle).
- Parse & Chip → 3D Chip: extruded units.

## Files and functions

Paths are relative to `src/CG_2310218L/` unless they start with another top-level folder.

| File | Functions / classes | Role |
|---|---|---|
| CO3_ObjectAlgorithms/Raster.h / .cpp | `bresenhamLine`, `midpointCircle`, `scanlineFill` | 2D scan conversion |
| CO3_ObjectAlgorithms/Extrude.h / .cpp | `prism`, `box` | 2D outline → 3D solid |

**Full explanation, worked example and viva questions:** [CO3_Explanation.pdf](CO3_Explanation.pdf)
