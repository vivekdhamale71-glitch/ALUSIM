// Sutherland-Hodgman clipping of a polygon against a window.

#include <vector>

// a polygon corner with a position and a colour; every attribute is interpolated when a corner is created on the window edge
struct Vertex {
    float x;
    float y;
    float r;
    float g;
    float b;
    float a;
};

struct Window {
    float left;
    float top;
    float right;
    float bottom;
};

// the signed distance of a corner to one window edge (0 left, 1 right, 2 top, 3 bottom); 0 or more means inside
static float distanceToEdge(const Vertex& p, const Window& w, int edge) {
    if (edge == 0) {
        return p.x - w.left;
    }
    if (edge == 1) {
        return w.right - p.x;
    }
    if (edge == 2) {
        return p.y - w.top;
    }
    return w.bottom - p.y;
}

// one value between two: a + (b - a) * t
static float between(float a, float b, float t) {
    return a + (b - a) * t;
}

// the point where the edge p to q crosses the window edge, found from the two signed distances;
// the colour is interpolated with the same t as the position
Vertex crossing(const Vertex& p, const Vertex& q, float dp, float dq) {
    float t = dp / (dp - dq);
    Vertex v = {between(p.x, q.x, t), between(p.y, q.y, t), between(p.r, q.r, t),
                between(p.g, q.g, t), between(p.b, q.b, t), between(p.a, q.a, t)};
    return v;
}

// clips against one window edge: walk the polygon from the previous corner to the current corner
//   in to in: keep the current corner;  in to out: keep the crossing
//   out to in: keep the crossing and the current corner;  out to out: keep nothing
std::vector<Vertex> clipAgainstEdge(const std::vector<Vertex>& polygon, const Window& w, int edge) {
    std::vector<Vertex> result;
    for (std::size_t i = 0; i < polygon.size(); ++i) {
        const Vertex& current = polygon[i];
        const Vertex& previous = polygon[(i + polygon.size() - 1) % polygon.size()];
        float dc = distanceToEdge(current, w, edge);
        float dp = distanceToEdge(previous, w, edge);
        if (dc >= 0.0f) {
            if (dp < 0.0f) {
                result.push_back(crossing(previous, current, dp, dc));
            }
            result.push_back(current);
        } else if (dp >= 0.0f) {
            result.push_back(crossing(previous, current, dp, dc));
        }
    }
    return result;
}

// Sutherland-Hodgman: the output of each edge is the input of the next; the result is empty if nothing is inside
std::vector<Vertex> sutherlandHodgman(const std::vector<Vertex>& polygon, const Window& w) {
    std::vector<Vertex> current = polygon;
    for (int edge = 0; edge < 4 && !current.empty(); ++edge) {
        current = clipAgainstEdge(current, w, edge);
    }
    return current;
}

// the area of a polygon by the shoelace formula, which shows how much of a shape survived the clipping
float polygonArea(const std::vector<Vertex>& polygon) {
    float sum = 0.0f;
    for (std::size_t i = 0; i < polygon.size(); ++i) {
        const Vertex& a = polygon[i];
        const Vertex& b = polygon[(i + 1) % polygon.size()];
        sum += a.x * b.y - b.x * a.y;
    }
    if (sum < 0.0f) {
        sum = -sum;
    }
    return sum / 2.0f;
}
