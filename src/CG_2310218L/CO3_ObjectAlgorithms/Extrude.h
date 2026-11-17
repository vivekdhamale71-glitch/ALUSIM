// =====================================================================
//  2D -> 3D objects by extrusion (sweep)                    [CG CO3]
//  The 3D chip is built from the same 2D outlines as the datapath
//  diagram: each outline is swept upwards into a solid prism.
// =====================================================================
#pragma once
#include <vector>

#include "CG_2310218L/CO2_Transformations/Math.h"

namespace vas::gfx {

// sweep a 2D outline (x, z on the board, counter-clockwise from above) up
// to height h; the top is a triangle fan from a point that sees every vertex
void prism(const std::vector<Vec2>& outline, float h, Vec2 fanCenter);

// axis-aligned box from y0 to y1: a prism of a rectangle
void box(float x0, float z0, float x1, float z1, float y0, float y1);

} // namespace vas::gfx
