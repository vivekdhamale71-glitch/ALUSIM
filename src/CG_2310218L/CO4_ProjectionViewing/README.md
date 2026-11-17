# 2310218L.CO4 — Apply various methods for projection and viewing it into real world.

Every 3D view sets up a perspective camera with gluPerspective and gluLookAt. The 3D chip has an orbit camera and an animated 3D ↔ 2D switch: a dolly zoom narrows the field of view while the eye backs away until the projection becomes parallel (glOrtho top view). gluProject maps 3D points back to the screen for crisp labels.

## Where you see it in the simulator

- Parse & Chip → 3D Chip: drag to orbit, wheel to zoom, 3D / 2D buttons.
- Pipeline (3D conveyor), Arithmetic (register cubes), Data Lab (3D bars) all use Scene3D.

## Files and functions

Paths are relative to `src/CG_2310218L/` unless they start with another top-level folder.

| File | Functions / classes | Role |
|---|---|---|
| CO4_ProjectionViewing/Camera.h / .cpp | `orbitCamera` | perspective ↔ orthographic dolly zoom + gluLookAt |
| CO4_ProjectionViewing/Scene3D.h / .cpp | `begin`, `end`, `box`, `project` | 3D panel inside the 2D UI |

**Full explanation, worked example and viva questions:** [CO4_Explanation.pdf](CO4_Explanation.pdf)
