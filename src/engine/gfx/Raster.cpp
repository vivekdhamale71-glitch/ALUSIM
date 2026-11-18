#include "engine/gfx/Raster.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace vas::gfx {

std::vector<IPoint> bresenhamLine(int x0, int y0, int x1, int y1) {
    std::vector<IPoint> pts;
    int dx = std::abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -std::abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    pts.reserve(std::size_t(std::max(dx, -dy) + 1));
    while (true) {
        pts.push_back({x0, y0});
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
    return pts;
}

std::vector<IPoint> midpointCircle(int cx, int cy, int r) {
    std::vector<IPoint> pts;
    int x = 0, y = r, d = 1 - r;
    auto plot8 = [&](int px, int py) {
        pts.push_back({cx + px, cy + py}); pts.push_back({cx - px, cy + py});
        pts.push_back({cx + px, cy - py}); pts.push_back({cx - px, cy - py});
        pts.push_back({cx + py, cy + px}); pts.push_back({cx - py, cy + px});
        pts.push_back({cx + py, cy - px}); pts.push_back({cx - py, cy - px});
    };
    while (x <= y) {
        plot8(x, y);
        if (d < 0) d += 2 * x + 3;
        else { d += 2 * (x - y) + 5; --y; }
        ++x;
    }
    return pts;
}

std::vector<Span> scanlineFill(const std::vector<Vec2>& poly) {
    std::vector<Span> spans;
    if (poly.size() < 3) return spans;
    float ymin = poly[0].y, ymax = poly[0].y;
    for (const auto& p : poly) { ymin = std::min(ymin, p.y); ymax = std::max(ymax, p.y); }
    std::vector<float> xs;
    for (int y = int(std::ceil(ymin)); y <= int(std::floor(ymax)); ++y) {
        float sy = y + 0.5f;   // sample at pixel centre
        xs.clear();
        for (std::size_t i = 0; i < poly.size(); ++i) {
            Vec2 a = poly[i], b = poly[(i + 1) % poly.size()];
            if ((a.y <= sy && b.y > sy) || (b.y <= sy && a.y > sy)) {
                float t = (sy - a.y) / (b.y - a.y);
                xs.push_back(a.x + t * (b.x - a.x));
            }
        }
        std::sort(xs.begin(), xs.end());
        for (std::size_t i = 0; i + 1 < xs.size(); i += 2) spans.push_back({y, xs[i], xs[i + 1]});
    }
    return spans;
}

} // namespace vas::gfx
