// =====================================================================
//  Line clipping                                          [CG CO5]
//   * Cohen-Sutherland - when the datapath diagram is zoomed or panned,
//     every wire segment is clipped against the diagram's viewport so
//     nothing is drawn over the neighbouring panels.
//   * Liang-Barsky     - the waveform panel scrolls left as cycles go
//     by; each signal trace segment is clipped to the visible window.
// =====================================================================
#pragma once
#include "CG_2310218L/CO2_Transformations/Math.h"

namespace vas::gfx {

// returns false if the segment is completely outside; otherwise a and b
// are moved onto the clip rectangle
bool cohenSutherland(Vec2& a, Vec2& b, const Rect& clip);
bool liangBarsky(Vec2& a, Vec2& b, const Rect& clip);

} // namespace vas::gfx
