#include "engine/gfx/Clip.h"

namespace vas::gfx {

// screen y grows downwards, so "top" is y < window.y
int outCode(Vec2 p, const Rect& c) {
    int code = CLIP_INSIDE;
    if (p.x < c.x) code |= CLIP_LEFT;
    else if (p.x > c.r()) code |= CLIP_RIGHT;
    if (p.y < c.y) code |= CLIP_TOP;
    else if (p.y > c.b()) code |= CLIP_BOTTOM;
    return code;
}

bool cohenSutherland(Vec2& a, Vec2& b, const Rect& c) {
    int ca = outCode(a, c), cb = outCode(b, c);
    while (true) {
        if (!(ca | cb)) return true;      // both inside: trivially accept
        if (ca & cb) return false;        // both beyond the same edge: reject
        int out = ca ? ca : cb;           // an end point that is outside
        Vec2 p;
        if (out & CLIP_TOP)         { p.x = a.x + (b.x - a.x) * (c.y - a.y) / (b.y - a.y);   p.y = c.y; }
        else if (out & CLIP_BOTTOM) { p.x = a.x + (b.x - a.x) * (c.b() - a.y) / (b.y - a.y); p.y = c.b(); }
        else if (out & CLIP_RIGHT)  { p.y = a.y + (b.y - a.y) * (c.r() - a.x) / (b.x - a.x); p.x = c.r(); }
        else                        { p.y = a.y + (b.y - a.y) * (c.x - a.x) / (b.x - a.x);   p.x = c.x; }
        if (out == ca) { a = p; ca = outCode(a, c); }
        else           { b = p; cb = outCode(b, c); }
    }
}

// for each edge k:  p_k * t <= q_k
//   p < 0 : the line enters through this edge  -> t0 = max(t0, q / p)
//   p > 0 : the line leaves through this edge  -> t1 = min(t1, q / p)
bool liangBarskyT(Vec2 a, Vec2 b, const Rect& c, float& t0, float& t1) {
    float dx = b.x - a.x, dy = b.y - a.y;
    float p[4] = {-dx, dx, -dy, dy};
    float q[4] = {a.x - c.x, c.r() - a.x, a.y - c.y, c.b() - a.y};
    t0 = 0;
    t1 = 1;
    for (int i = 0; i < 4; ++i) {
        if (p[i] == 0) {
            if (q[i] < 0) return false;   // parallel to this edge and outside it
            continue;
        }
        float t = q[i] / p[i];
        if (p[i] < 0) { if (t > t1) return false; if (t > t0) t0 = t; }
        else          { if (t < t0) return false; if (t < t1) t1 = t; }
    }
    return true;
}

bool liangBarsky(Vec2& a, Vec2& b, const Rect& c) {
    float t0, t1;
    if (!liangBarskyT(a, b, c, t0, t1)) return false;
    Vec2 d = b - a;
    Vec2 na = a + d * t0, nb = a + d * t1;
    a = na;
    b = nb;
    return true;
}

} // namespace vas::gfx
