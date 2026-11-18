// Scan-line fill, boundary fill and flood fill.

#include <algorithm>
#include <cmath>
#include <vector>

struct Point2 {
    float x;
    float y;
};

// the filled part of one pixel row: from x0 to x1 on row y
struct Span {
    int y;
    float x0;
    float x1;
};

// scan-line fill: for every row find where the polygon edges cross it, sort the crossings,
// and fill between the 1st and 2nd, the 3rd and 4th, and so on (inside, outside, inside, ...)
std::vector<Span> scanlineFill(const std::vector<Point2>& polygon) {
    std::vector<Span> spans;
    if (polygon.size() < 3) {
        return spans;
    }
    float lowest = polygon[0].y;
    float highest = polygon[0].y;
    for (std::size_t i = 0; i < polygon.size(); ++i) {
        lowest = std::min(lowest, polygon[i].y);
        highest = std::max(highest, polygon[i].y);
    }
    for (int row = static_cast<int>(std::ceil(lowest)); row <= static_cast<int>(std::floor(highest)); ++row) {
        float sampleY = static_cast<float>(row) + 0.5f;
        std::vector<float> crossings;
        for (std::size_t i = 0; i < polygon.size(); ++i) {
            Point2 a = polygon[i];
            Point2 b = polygon[(i + 1) % polygon.size()];
            bool goesUp = a.y <= sampleY && b.y > sampleY;
            bool goesDown = b.y <= sampleY && a.y > sampleY;
            if (goesUp || goesDown) {
                float t = (sampleY - a.y) / (b.y - a.y);
                crossings.push_back(a.x + t * (b.x - a.x));
            }
        }
        std::sort(crossings.begin(), crossings.end());
        for (std::size_t i = 0; i + 1 < crossings.size(); i += 2) {
            Span s = {row, crossings[i], crossings[i + 1]};
            spans.push_back(s);
        }
    }
    return spans;
}

// a small grid of pixels: 0 is empty, anything else is a colour
struct Grid {
    int width;
    int height;
    std::vector<int> cells;
};

// the colour of a pixel, or -1 outside the grid
static int cellAt(const Grid& g, int x, int y) {
    if (x < 0 || y < 0 || x >= g.width || y >= g.height) {
        return -1;
    }
    return g.cells[static_cast<std::size_t>(y) * g.width + x];
}

// boundary fill: spread from a seed to the four neighbours and stop at pixels that have the boundary colour
void boundaryFill(Grid& g, int x, int y, int fillColor, int boundaryColor) {
    std::vector<Point2> stack;
    Point2 seed = {static_cast<float>(x), static_cast<float>(y)};
    stack.push_back(seed);
    while (!stack.empty()) {
        Point2 p = stack.back();
        stack.pop_back();
        int px = static_cast<int>(p.x);
        int py = static_cast<int>(p.y);
        int c = cellAt(g, px, py);
        if (c == -1 || c == boundaryColor || c == fillColor) {
            continue;
        }
        g.cells[static_cast<std::size_t>(py) * g.width + px] = fillColor;
        Point2 next[4] = {{p.x + 1, p.y}, {p.x - 1, p.y}, {p.x, p.y + 1}, {p.x, p.y - 1}};
        for (int i = 0; i < 4; ++i) {
            stack.push_back(next[i]);
        }
    }
}

// flood fill: replace the colour of the seed pixel, and of every pixel joined to it that has the same old colour
void floodFill(Grid& g, int x, int y, int newColor) {
    int oldColor = cellAt(g, x, y);
    if (oldColor == -1 || oldColor == newColor) {
        return;
    }
    std::vector<Point2> stack;
    Point2 seed = {static_cast<float>(x), static_cast<float>(y)};
    stack.push_back(seed);
    while (!stack.empty()) {
        Point2 p = stack.back();
        stack.pop_back();
        int px = static_cast<int>(p.x);
        int py = static_cast<int>(p.y);
        if (cellAt(g, px, py) != oldColor) {
            continue;
        }
        g.cells[static_cast<std::size_t>(py) * g.width + px] = newColor;
        Point2 next[4] = {{p.x + 1, p.y}, {p.x - 1, p.y}, {p.x, p.y + 1}, {p.x, p.y - 1}};
        for (int i = 0; i < 4; ++i) {
            stack.push_back(next[i]);
        }
    }
}
