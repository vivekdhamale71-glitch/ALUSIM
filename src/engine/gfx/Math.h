// =====================================================================
//  2D math for the renderer                       [CG CO2, PSOOP]
//  (animation helpers clampf / ease* / approach are used by every view)
//  Vec2 / Color with overloaded operators and a 3x3 homogeneous
//  matrix used for the datapath pan / zoom (translate + scale) and to
//  map world coordinates to screen space before clipping.
// =====================================================================
#pragma once
#include <algorithm>
#include <cmath>

namespace vas::gfx {

// set by the theme: on light backgrounds "emphasis" means darker, not lighter
inline bool gLightBackground = false;

struct Vec2 {
    float x = 0, y = 0;
    Vec2() = default;
    Vec2(float x_, float y_) : x(x_), y(y_) {}
    Vec2 operator+(const Vec2& o) const { return {x + o.x, y + o.y}; }
    Vec2 operator-(const Vec2& o) const { return {x - o.x, y - o.y}; }
    Vec2 operator*(float s) const { return {x * s, y * s}; }
    Vec2 operator/(float s) const { return {x / s, y / s}; }
    Vec2& operator+=(const Vec2& o) { x += o.x; y += o.y; return *this; }
    Vec2& operator-=(const Vec2& o) { x -= o.x; y -= o.y; return *this; }
    float length() const { return std::sqrt(x * x + y * y); }
    Vec2 normalized() const { float l = length(); return l > 1e-6f ? Vec2(x / l, y / l) : Vec2(); }
    Vec2 perp() const { return {-y, x}; }
};

struct Rect {
    float x = 0, y = 0, w = 0, h = 0;
    Rect() = default;
    Rect(float x_, float y_, float w_, float h_) : x(x_), y(y_), w(w_), h(h_) {}
    float r() const { return x + w; }
    float b() const { return y + h; }
    Vec2 center() const { return {x + w / 2, y + h / 2}; }
    bool contains(Vec2 p) const { return p.x >= x && p.x < x + w && p.y >= y && p.y < y + h; }
    Rect inset(float d) const { return {x + d, y + d, w - 2 * d, h - 2 * d}; }
    Rect inset(float dx, float dy) const { return {x + dx, y + dy, w - 2 * dx, h - 2 * dy}; }
    Rect cutTop(float a) { Rect t{x, y, w, a}; y += a; h -= a; return t; }
    Rect cutBottom(float a) { Rect t{x, y + h - a, w, a}; h -= a; return t; }
    Rect cutLeft(float a) { Rect t{x, y, a, h}; x += a; w -= a; return t; }
    Rect cutRight(float a) { Rect t{x + w - a, y, a, h}; w -= a; return t; }
    Rect intersect(const Rect& o) const {
        float nx = std::max(x, o.x), ny = std::max(y, o.y);
        float nr = std::min(r(), o.r()), nb = std::min(b(), o.b());
        return {nx, ny, std::max(0.f, nr - nx), std::max(0.f, nb - ny)};
    }
};

struct Color {
    float r = 1, g = 1, b = 1, a = 1;
    Color() = default;
    constexpr Color(float r_, float g_, float b_, float a_ = 1) : r(r_), g(g_), b(b_), a(a_) {}
    static Color hex(unsigned v, float a = 1) {
        return {((v >> 16) & 255) / 255.f, ((v >> 8) & 255) / 255.f, (v & 255) / 255.f, a};
    }
    Color alpha(float na) const { return {r, g, b, a * na}; }
    Color operator*(float s) const { return {r * s, g * s, b * s, a}; }
    Color lerp(const Color& o, float t) const {
        return {r + (o.r - r) * t, g + (o.g - g) * t, b + (o.b - b) * t, a + (o.a - a) * t};
    }
    Color lighten(float t) const {
        return gLightBackground ? lerp(Color(0, 0, 0, a), t * 0.55f) : lerp(Color(1, 1, 1, a), t);
    }
};

// 3x3 matrix for 2D homogeneous transforms (row-major)      [CG CO2]
//   | m0 m1 m2 |   | x |      translate: m2, m5      scale: m0, m4
//   | m3 m4 m5 | * | y |      rotate:    m0 m1 / m3 m4 = cos -sin / sin cos
//   |  0  0  1 |   | 1 |
// Bodies are in Math.cpp.
struct Mat3 {
    float m[9] = {1, 0, 0, 0, 1, 0, 0, 0, 1};
    static Mat3 translate(float tx, float ty);
    static Mat3 scale(float sx, float sy);
    static Mat3 rotate(float rad);
    Mat3 operator*(const Mat3& o) const;      // composition: (A * B) applies B first
    Vec2 apply(Vec2 p) const;                  // transform a point
    float scaleX() const;                      // current zoom factor
    Mat3 inverseAffine() const;                // screen -> world (for the mouse)
};

inline float clampf(float v, float a, float b) { return v < a ? a : (v > b ? b : v); }
inline float lerpf(float a, float b, float t) { return a + (b - a) * t; }
inline float easeOutCubic(float t) { t = clampf(t, 0, 1); float u = 1 - t; return 1 - u * u * u; }
inline float easeInOutCubic(float t) {
    t = clampf(t, 0, 1);
    return t < 0.5f ? 4 * t * t * t : 1 - std::pow(-2 * t + 2, 3) / 2;
}
inline float easeOutBack(float t) {
    t = clampf(t, 0, 1);
    const float c1 = 1.70158f, c3 = c1 + 1;
    return 1 + c3 * std::pow(t - 1, 3) + c1 * std::pow(t - 1, 2);
}
// frame-rate independent exponential approach
inline float approach(float cur, float target, float speed, float dt) {
    return cur + (target - cur) * (1 - std::exp(-speed * dt));
}

} // namespace vas::gfx
