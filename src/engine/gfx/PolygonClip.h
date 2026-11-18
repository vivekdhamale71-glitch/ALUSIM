// =====================================================================
//  Sutherland-Hodgman polygon clipping                       [CG CO5]
//  Clips a convex polygon against the four edges of a window, one edge
//  at a time. Each vertex carries its colour and texture coordinate,
//  which are interpolated at the new intersection points, so clipped
//  gradients and clipped text stay correct.
// =====================================================================
#pragma once
#include <vector>

#include "engine/gfx/Math.h"

namespace vas::gfx {

struct ClipVertex {
    float x, y;
    float r, g, b, a;          // colour
    float u = 0, v = 0;        // texture coordinate
};

std::vector<ClipVertex> sutherlandHodgman(const std::vector<ClipVertex>& poly, const Rect& window);

} // namespace vas::gfx
