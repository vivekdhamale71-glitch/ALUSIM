#include "CG_2310218L/CO5_Clipping/Clip.h"

namespace vas::gfx {

namespace {
enum : int { INSIDE = 0, LEFT = 1, RIGHT = 2, BOTTOM = 4, TOP = 8 };

int outCode(Vec2 p, const Rect& c) {
    int code = INSIDE;
    if (p.x < c.x) code |= LEFT;
    else if (p.x > c.r()) code |= RIGHT;
    if (p.y < c.y) code |= TOP;
    else if (p.y > c.b()) code |= BOTTOM;
    return code;
}
} // namespace

bool cohenSutherland(Vec2& a, Vec2& b, const Rect& c) {
    int ca = outCode(a, c), cb = outCode(b, c);
    while (true) {
        if (!(ca | cb)) return true;      // both inside: trivially accept
        if (ca & cb) return false;        // share an outside region: reject
        int out = ca ? ca : cb;
        Vec2 p;
        if (out & TOP) { p.x = a.x + (b.x - a.x) * (c.y - a.y) / (b.y - a.y); p.y = c.y; }
        else if (out & BOTTOM) { p.x = a.x + (b.x - a.x) * (c.b() - a.y) / (b.y - a.y); p.y = c.b(); }
        else if (out & RIGHT) { p.y = a.y + (b.y - a.y) * (c.r() - a.x) / (b.x - a.x); p.x = c.r(); }
        else { p.y = a.y + (b.y - a.y) * (c.x - a.x) / (b.x - a.x); p.x = c.x; }
        if (out == ca) { a = p; ca = outCode(a, c); }
        else { b = p; cb = outCode(b, c); }
    }
}

bool liangBarsky(Vec2& a, Vec2& b, const Rect& c) {
    float dx = b.x - a.x, dy = b.y - a.y;
    float p[4] = {-dx, dx, -dy, dy};
    float q[4] = {a.x - c.x, c.r() - a.x, a.y - c.y, c.b() - a.y};
    float u1 = 0, u2 = 1;
    for (int i = 0; i < 4; ++i) {
        if (p[i] == 0) {
            if (q[i] < 0) return false;   // parallel and outside
            continue;
        }
        float t = q[i] / p[i];
        if (p[i] < 0) { if (t > u2) return false; if (t > u1) u1 = t; }
        else { if (t < u1) return false; if (t < u2) u2 = t; }
    }
    Vec2 na{a.x + u1 * dx, a.y + u1 * dy};
    Vec2 nb{a.x + u2 * dx, a.y + u2 * dy};
    a = na;
    b = nb;
    return true;
}

} // namespace vas::gfx
