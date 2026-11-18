#include "engine/gfx/PolygonClip.h"

namespace vas::gfx {

namespace {

// signed distance to window edge e (0 left, 1 right, 2 top, 3 bottom); >= 0 is inside
float inside(const ClipVertex& p, const Rect& w, int e) {
    switch (e) {
        case 0: return p.x - w.x;
        case 1: return w.r() - p.x;
        case 2: return p.y - w.y;
        default: return w.b() - p.y;
    }
}

// the point where edge p -> q crosses the window edge; every attribute is
// interpolated with the same t
ClipVertex crossing(const ClipVertex& p, const ClipVertex& q, float dp, float dq) {
    float t = dp / (dp - dq);
    auto L = [t](float a, float b) { return a + (b - a) * t; };
    return {L(p.x, q.x), L(p.y, q.y), L(p.r, q.r), L(p.g, q.g), L(p.b, q.b), L(p.a, q.a), L(p.u, q.u), L(p.v, q.v)};
}

} // namespace

// for every edge, walk the polygon's edges prev -> cur:
//   in  -> in  : keep cur
//   in  -> out : keep the crossing
//   out -> in  : keep the crossing, then cur
//   out -> out : keep nothing
std::vector<ClipVertex> sutherlandHodgman(const std::vector<ClipVertex>& in, const Rect& w) {
    std::vector<ClipVertex> poly = in, out;
    for (int e = 0; e < 4 && !poly.empty(); ++e) {
        out.clear();
        for (std::size_t i = 0; i < poly.size(); ++i) {
            const ClipVertex& cur = poly[i];
            const ClipVertex& prev = poly[(i + poly.size() - 1) % poly.size()];
            float dc = inside(cur, w, e), dp = inside(prev, w, e);
            if (dc >= 0) {
                if (dp < 0) out.push_back(crossing(prev, cur, dp, dc));
                out.push_back(cur);
            } else if (dp >= 0) {
                out.push_back(crossing(prev, cur, dp, dc));
            }
        }
        poly.swap(out);
    }
    return poly;
}

} // namespace vas::gfx
