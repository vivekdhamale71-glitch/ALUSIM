# 2310218L.CO1 — Apply OpenGL graphics primitives to develop graphics applications.

The whole user interface (cards, buttons, wires, waveform, LEDs, text) is drawn with OpenGL primitives; no UI library is used.

## Where you see it in the simulator

- Every screen: rounded cards are GL_TRIANGLE_FAN, gradients GL_QUADS with per-vertex colour, wires GL_LINES / GL_LINE_STRIP, signal dots GL_POINTS, text textured GL_QUADS.

## Files and functions

Paths are relative to `src/CG_2310218L/` unless they start with another top-level folder.

| File | Functions / classes | Role |
|---|---|---|
| CO1_Primitives/Draw.h / Draw.cpp | `beginFrame`, `rect`, `rectGrad`, `roundRect`, `line`, `polyline`, `circle`, `ring`, `triangle`, `points`, `bezier`, `pushClip` | the drawing layer |
| CO1_Primitives/GL.h | includes | OpenGL / GLU / GLUT headers for every platform |

**Full explanation, worked example and viva questions:** [CO1_Explanation.pdf](CO1_Explanation.pdf)
