// =====================================================================
//  Line clipping against an axis-aligned window              [CG CO5]
//   * Cohen-Sutherland - 4-bit region codes: trivial accept / reject,
//     otherwise move an outside end point onto the edge it crosses.
//     outCode() is also what the Clipper uses to classify every line
//     the interface draws (inside / outside / crossing).
//   * Liang-Barsky     - parametric P(t) = a + t(b - a): the visible
//     part is t0 <= t <= t1. Dashed lines use the t range directly so
//     no dash outside the window is ever generated; the waveform traces
//     are clipped to the plot with it.
// =====================================================================
#pragma once
#include "engine/gfx/Math.h"

namespace vas::gfx {

enum : int { CLIP_INSIDE = 0, CLIP_LEFT = 1, CLIP_RIGHT = 2, CLIP_BOTTOM = 4, CLIP_TOP = 8 };

int outCode(Vec2 p, const Rect& window);

// false if the segment is completely outside; otherwise a and b are moved
// onto the window
bool cohenSutherland(Vec2& a, Vec2& b, const Rect& window);
bool liangBarsky(Vec2& a, Vec2& b, const Rect& window);

// Liang-Barsky parameters only: visible part is a + t(b - a), t0 <= t <= t1
bool liangBarskyT(Vec2 a, Vec2 b, const Rect& window, float& t0, float& t1);

} // namespace vas::gfx
