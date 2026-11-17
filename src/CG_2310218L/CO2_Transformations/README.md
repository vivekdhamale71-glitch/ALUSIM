# 2310218L.CO2 — Solve the real time problems using geometric transformations.

The datapath diagram can be zoomed and panned in real time. Its view is a 3 × 3 homogeneous matrix M = T(centre + pan) · S(zoom) · T(-world centre); its inverse maps the mouse back so the zoom stays under the cursor. The 3D views use the OpenGL matrix stack (glTranslatef, glRotatef, glScalef).

## Where you see it in the simulator

- Datapath screen: mouse wheel zooms about the cursor, drag pans.
- 3D views: the chip rotates, register cubes flip (glRotatef), packets and bars translate.

## Files and functions

Paths are relative to `src/CG_2310218L/` unless they start with another top-level folder.

| File | Functions / classes | Role |
|---|---|---|
| CO2_Transformations/Math.h / Math.cpp | `Mat3::translate`, `scale`, `rotate`, `operator*`, `apply`, `inverseAffine` | 2D homogeneous transforms |
| app/DatapathView.cpp | view matrix | builds and applies M |
| CO4 Scene3D.cpp, app/ChipView.cpp | glTranslatef / glRotatef / glScalef | 3D transforms on the matrix stack |

**Full explanation, worked example and viva questions:** [CO2_Explanation.pdf](CO2_Explanation.pdf)
