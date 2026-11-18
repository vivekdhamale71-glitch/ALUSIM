// =====================================================================
//  Clipper - the clipping stage of the 2D interface          [CG CO5]
//  Every card, button, wire, glyph and icon the interface draws goes
//  through here before it reaches OpenGL; the 2D interface does not use
//  glScissor. The window is the innermost rectangle pushed with
//  draw::pushClip (a card, a list, the datapath view ...).
//    polygon()  bounding box inside   -> drawn as is   (trivial accept)
//               bounding box outside  -> dropped       (trivial reject)
//               otherwise             -> Sutherland-Hodgman
//    lineClass() Cohen-Sutherland outcodes of a line's end points
// =====================================================================
#pragma once
#include <vector>

#include "engine/gfx/PolygonClip.h"

namespace vas::gfx::clipper {

void setWindow(const Rect* window);     // nullptr: no clipping
const Rect* window();

// convex polygon, drawn as a GL_TRIANGLE_FAN after clipping
void polygon(const std::vector<ClipVertex>& poly, bool textured = false);

enum LineClass { REJECT, ACCEPT, CROSSING };
// classify the segment a-b of half-width pad against the window
LineClass lineClass(Vec2 a, Vec2 b, float pad);

struct Stats {
    int accepted = 0, clipped = 0, rejected = 0;
};
Stats& stats();                         // reset every frame by draw::beginFrame

} // namespace vas::gfx::clipper
