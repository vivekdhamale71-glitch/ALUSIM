// =====================================================================
//  Scan-conversion algorithms                             [CG CO3]
//  Used where the drawing needs them, not as a separate exercise:
//   * Bresenham line     - the active datapath wires are rasterised on
//                          the pixel grid and drawn as a stream of
//                          glowing "signal" dots that flow along them
//   * Midpoint circle    - the ring of each status-flag LED (CF ZF SF
//                          OF) and the wire-junction nodes
//   * Scanline fill      - fills the ALU and multiplexer symbols (non-
//                          rectangular polygons) with a vertical
//                          gradient, one horizontal span per scanline
// =====================================================================
#pragma once
#include <vector>

#include "engine/gfx/Math.h"

namespace vas::gfx {

struct IPoint {
    int x, y;
};

// integer Bresenham line from (x0,y0) to (x1,y1), all octants
std::vector<IPoint> bresenhamLine(int x0, int y0, int x1, int y1);

// midpoint circle: all boundary pixels of a circle of radius r
std::vector<IPoint> midpointCircle(int cx, int cy, int r);

struct Span {
    int y;
    float x0, x1;
};
// scanline polygon fill: horizontal spans covering the polygon interior
std::vector<Span> scanlineFill(const std::vector<Vec2>& poly);

} // namespace vas::gfx
