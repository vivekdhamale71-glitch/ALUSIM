#include "engine/gfx/Clipper.h"

#include <algorithm>

#include "engine/gfx/GL.h"
#include "engine/gfx/Clip.h"

namespace vas::gfx::clipper {

namespace {
bool gHas = false;
Rect gWin;
Stats gStats;

void emit(const std::vector<ClipVertex>& p, bool textured) {
    glBegin(GL_TRIANGLE_FAN);
    for (const ClipVertex& v : p) {
        glColor4f(v.r, v.g, v.b, v.a);
        if (textured) glTexCoord2f(v.u, v.v);
        glVertex2f(v.x, v.y);
    }
    glEnd();
}
} // namespace

void setWindow(const Rect* w) {
    gHas = w != nullptr;
    if (w) gWin = *w;
}

const Rect* window() { return gHas ? &gWin : nullptr; }

Stats& stats() { return gStats; }

void polygon(const std::vector<ClipVertex>& p, bool textured) {
    if (p.size() < 3) return;
    if (!gHas) { emit(p, textured); return; }
    float x0 = p[0].x, x1 = p[0].x, y0 = p[0].y, y1 = p[0].y;
    for (const ClipVertex& v : p) {
        x0 = std::min(x0, v.x); x1 = std::max(x1, v.x);
        y0 = std::min(y0, v.y); y1 = std::max(y1, v.y);
    }
    if (x0 >= gWin.x && x1 <= gWin.r() && y0 >= gWin.y && y1 <= gWin.b()) {   // trivially inside
        ++gStats.accepted;
        emit(p, textured);
        return;
    }
    if (x1 < gWin.x || x0 > gWin.r() || y1 < gWin.y || y0 > gWin.b() || gWin.w <= 0 || gWin.h <= 0) {
        ++gStats.rejected;                                                       // trivially outside
        return;
    }
    std::vector<ClipVertex> c = sutherlandHodgman(p, gWin);
    ++gStats.clipped;
    if (c.size() >= 3) emit(c, textured);
}

// Cohen-Sutherland outcodes: a thick line is fully inside if its centre
// line is inside the window shrunk by its half-width, fully outside if both
// end points lie beyond the same edge of the window grown by it
LineClass lineClass(Vec2 a, Vec2 b, float pad) {
    if (!gHas) return ACCEPT;
    Rect grown(gWin.x - pad, gWin.y - pad, gWin.w + 2 * pad, gWin.h + 2 * pad);
    if (outCode(a, grown) & outCode(b, grown)) {
        ++gStats.rejected;
        return REJECT;
    }
    Rect shrunk = gWin.inset(pad);
    if (shrunk.w > 0 && shrunk.h > 0 && !(outCode(a, shrunk) | outCode(b, shrunk))) return ACCEPT;
    return CROSSING;
}

} // namespace vas::gfx::clipper
