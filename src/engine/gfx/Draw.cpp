#include "engine/gfx/Draw.h"

#include <cmath>

#include "engine/gfx/GL.h"
#include "engine/gfx/Clip.h"
#include "engine/gfx/Clipper.h"

namespace vas::gfx::draw {

static int gW = 1, gH = 1;
static std::vector<Rect> gClip;
static bool gLight = false;

void setLightMode(bool light) {
    gLight = light;
    gLightBackground = light;
}

void beginFrame(int w, int h) {
    gW = w;
    gH = h;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, w, h, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_SCISSOR_TEST);            // 2D clipping is done in software (CG CO5)
    gClip.clear();
    clipper::setWindow(nullptr);
    clipper::stats() = {};
}
int screenW() { return gW; }
int screenH() { return gH; }

// Every shape is built as a convex polygon of coloured vertices and handed
// to the clipper (CG CO5), which draws it as a GL_TRIANGLE_FAN.
static ClipVertex V(float x, float y, const Color& c) { return {x, y, c.r, c.g, c.b, c.a}; }
static ClipVertex V(Vec2 p, const Color& c) { return V(p.x, p.y, c); }
static void quad(Vec2 a, Vec2 b, Vec2 c, Vec2 d, const Color& col) {
    clipper::polygon({V(a, col), V(b, col), V(c, col), V(d, col)});
}

void rect(const Rect& r, const Color& c) {
    quad({r.x, r.y}, {r.r(), r.y}, {r.r(), r.b()}, {r.x, r.b()}, c);
}

void rectGrad(const Rect& r, const Color& t, const Color& b) {
    clipper::polygon({V(r.x, r.y, t), V(r.r(), r.y, t), V(r.r(), r.b(), b), V(r.x, r.b(), b)});
}

void rectGradH(const Rect& r, const Color& l, const Color& rr) {
    clipper::polygon({V(r.x, r.y, l), V(r.r(), r.y, rr), V(r.r(), r.b(), rr), V(r.x, r.b(), l)});
}

// outline of a rounded rectangle as a point list (clockwise from top-left)
static int arcSegments(float rad) { return rad < 4 ? 2 : (rad < 10 ? 5 : 8); }

static std::vector<Vec2> roundPath(const Rect& r, float rad, int seg = -1) {
    rad = std::max(0.f, std::min(rad, std::min(r.w, r.h) / 2));
    std::vector<Vec2> p;
    if (seg < 0) seg = arcSegments(rad);
    auto arc = [&](float cx, float cy, float a0) {
        for (int i = 0; i <= seg; ++i) {
            float a = a0 + (float(M_PI) / 2) * i / seg;
            p.push_back({cx + std::cos(a) * rad, cy + std::sin(a) * rad});
        }
    };
    arc(r.x + rad, r.y + rad, float(M_PI));
    arc(r.r() - rad, r.y + rad, float(M_PI) * 1.5f);
    arc(r.r() - rad, r.b() - rad, 0);
    arc(r.x + rad, r.b() - rad, float(M_PI) * 0.5f);
    return p;
}

void roundRect(const Rect& r, float radius, const Color& c) { roundRectGrad(r, radius, c, c); }

void roundRectGrad(const Rect& r, float radius, const Color& top, const Color& bottom) {
    if (r.w <= 0 || r.h <= 0) return;
    auto p = roundPath(r, radius);              // a rounded rectangle is convex
    std::vector<ClipVertex> poly;
    poly.reserve(p.size());
    for (const Vec2& v : p) poly.push_back(V(v, top.lerp(bottom, (v.y - r.y) / r.h)));
    clipper::polygon(poly);
}

void roundRectOutline(const Rect& r, float radius, const Color& c, float width) {
    if (r.w <= 0 || r.h <= 0) return;
    Rect outer = r, inner = r.inset(width);
    // both paths must have the same vertex count or the strip folds over itself
    int seg = arcSegments(std::min(radius, std::min(r.w, r.h) / 2));
    auto po = roundPath(outer, radius, seg);
    auto pi = roundPath(inner, std::max(0.f, radius - width), seg);
    for (std::size_t i = 0; i < po.size(); ++i) {   // the band as convex quads
        std::size_t j = (i + 1) % po.size();
        quad(po[i], po[j], pi[j], pi[i], c);
    }
}

void shadow(const Rect& r, float radius, float spread, const Color& c) {
    const int layers = 10;
    for (int i = layers; i >= 1; --i) {
        float t = float(i) / layers;
        float e = spread * t;
        Rect rr(r.x - e, r.y - e + spread * 0.35f, r.w + 2 * e, r.h + 2 * e);
        roundRect(rr, radius + e, c.alpha((1 - t) * (1 - t) * (gLight ? 0.09f : 0.22f)));
    }
}

void glow(const Rect& r, float radius, float spread, const Color& c) {
    // additive light on dark themes; a soft tinted halo on light themes
    if (!gLight) glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    const int layers = 8;
    for (int i = layers; i >= 1; --i) {
        float t = float(i) / layers;
        float e = spread * t;
        roundRect(Rect(r.x - e, r.y - e, r.w + 2 * e, r.h + 2 * e), radius + e,
                  c.alpha((1 - t) * (gLight ? 0.10f : 0.16f)));
    }
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void line(Vec2 a, Vec2 b, const Color& c, float w) {
    Vec2 d = b - a;
    float len = d.length();
    if (len < 0.01f) return;
    if (clipper::lineClass(a, b, w * 0.5f) == clipper::REJECT) return;   // Cohen-Sutherland outcodes
    Vec2 n = d.perp() / len * (w * 0.5f);
    quad(a + n, b + n, b - n, a - n, c);
}

void polyline(const std::vector<Vec2>& pts, const Color& c, float w) {
    for (std::size_t i = 0; i + 1 < pts.size(); ++i) line(pts[i], pts[i + 1], c, w);
    if (w > 2.5f)
        for (std::size_t i = 1; i + 1 < pts.size(); ++i) circle(pts[i], w * 0.5f, c, 10);
}

void dashedLine(Vec2 a, Vec2 b, const Color& c, float w, float dash, float gap, float offset) {
    Vec2 d = b - a;
    float len = d.length();
    if (len < 0.01f) return;
    Vec2 u = d / len;
    // Liang-Barsky gives the visible part [v0, v1] of the line, so dashes are
    // only generated there (a zoomed-in line may be mostly off screen)
    float v0 = 0, v1 = len;
    if (const Rect* win = clipper::window()) {
        float t0, t1;
        if (!liangBarskyT(a, b, win->inset(-w), t0, t1)) return;
        v0 = t0 * len;
        v1 = t1 * len;
    }
    float period = dash + gap;
    float s = -std::fmod(offset, period);
    if (s > 0) s -= period;
    if (v0 > period) s += std::floor((v0 - s) / period) * period - period;   // skip hidden dashes
    for (; s < v1; s += period) {
        float t0 = std::max(0.f, s), t1 = std::min(len, s + dash);
        if (t1 > t0) line(a + u * t0, a + u * t1, c, w);
    }
}

Vec2 bezierPoint(Vec2 p0, Vec2 p1, Vec2 p2, Vec2 p3, float t) {
    float u = 1 - t;
    return p0 * (u * u * u) + p1 * (3 * u * u * t) + p2 * (3 * u * t * t) + p3 * (t * t * t);
}

void bezier(Vec2 p0, Vec2 p1, Vec2 p2, Vec2 p3, const Color& c, float w, int seg) {
    std::vector<Vec2> pts;
    for (int i = 0; i <= seg; ++i) pts.push_back(bezierPoint(p0, p1, p2, p3, float(i) / seg));
    polyline(pts, c, w);
}

void circle(Vec2 c, float r, const Color& colr, int seg) {
    std::vector<ClipVertex> poly;
    poly.reserve(std::size_t(seg));
    for (int i = 0; i < seg; ++i) {
        float a = 2 * float(M_PI) * i / seg;
        poly.push_back(V(c.x + std::cos(a) * r, c.y + std::sin(a) * r, colr));
    }
    clipper::polygon(poly);
}

void ring(Vec2 c, float r, float w, const Color& colr, int seg) {
    for (int i = 0; i < seg; ++i) {
        float a0 = 2 * float(M_PI) * i / seg, a1 = 2 * float(M_PI) * (i + 1) / seg;
        Vec2 d0(std::cos(a0), std::sin(a0)), d1(std::cos(a1), std::sin(a1));
        quad(c + d0 * r, c + d1 * r, c + d1 * (r - w), c + d0 * (r - w), colr);
    }
}

void triangle(Vec2 a, Vec2 b, Vec2 c, const Color& colr) {
    clipper::polygon({V(a, colr), V(b, colr), V(c, colr)});
}

void convexPolygon(const std::vector<Vec2>& pts, const Color& colr) {
    std::vector<ClipVertex> poly;
    for (const auto& p : pts) poly.push_back(V(p, colr));
    clipper::polygon(poly);
}

void arrowHead(Vec2 tip, Vec2 dir, float size, const Color& c) {
    Vec2 d = dir.normalized();
    Vec2 n = d.perp();
    triangle(tip, tip - d * size + n * (size * 0.55f), tip - d * size - n * (size * 0.55f), c);
}

void points(const std::vector<Vec2>& pts, float size, const Color& c) {
    glPointSize(size);
    glEnable(GL_POINT_SMOOTH);
    glColor4f(c.r, c.g, c.b, c.a);
    const Rect* win = clipper::window();
    glBegin(GL_POINTS);
    for (const auto& p : pts)                   // point clipping: inside test
        if (!win || win->contains(p)) glVertex2f(p.x, p.y);
    glEnd();
    glDisable(GL_POINT_SMOOTH);
}

// ------------------------------------------------------------------- text
float text(Face f, int px, float x, float y, const std::string& s, const Color& c, Align a) {
    Font& font = Fonts::get(f, px);
    float w = font.width(s);
    if (a == Align::Center) x -= w / 2;
    else if (a == Align::Right) x -= w;
    font.draw(std::round(x), y, s, c);
    return w;
}

float textWidth(Face f, int px, const std::string& s) { return Fonts::get(f, px).width(s); }

float textIn(Face f, int px, const Rect& r, const std::string& s, const Color& c, Align a, float pad) {
    Font& font = Fonts::get(f, px);
    float y = r.y + (r.h - font.height()) / 2;
    float x = a == Align::Left ? r.x + pad : a == Align::Right ? r.r() - pad : r.x + r.w / 2;
    return text(f, px, x, std::round(y), s, c, a);
}

float paragraph(Face f, int px, const Rect& r, const std::string& s, const Color& c, float spacing) {
    Font& font = Fonts::get(f, px);
    float lh = font.height() * spacing;
    float y = r.y;
    std::string line, word;
    auto flush = [&](bool force) {
        (void)force;
        if (!line.empty()) font.draw(r.x, y, line, c);
        y += lh;
        line.clear();
    };
    for (std::size_t i = 0; i <= s.size(); ++i) {
        char ch = i < s.size() ? s[i] : ' ';
        if (ch == '\n') {
            if (!word.empty()) { line += (line.empty() ? "" : " ") + word; word.clear(); }
            flush(true);
            continue;
        }
        if (ch == ' ') {
            if (word.empty()) continue;
            std::string cand = line.empty() ? word : line + " " + word;
            if (font.width(cand) > r.w && !line.empty()) { flush(false); line = word; }
            else line = cand;
            word.clear();
        } else {
            word += ch;
        }
    }
    if (!line.empty()) flush(false);
    return y - r.y;
}

std::string ellipsize(Face f, int px, const std::string& s, float maxW) {
    Font& font = Fonts::get(f, px);
    if (font.width(s) <= maxW) return s;
    std::string r = s;
    while (!r.empty() && font.width(r + "...") > maxW) r.pop_back();
    return r + "...";
}

// ------------------------------------------------------------------- clip
// the clip stack: nested rectangles intersect; the innermost one is the
// window the software clipper uses (CG CO5)
static void applyClip() { clipper::setWindow(gClip.empty() ? nullptr : &gClip.back()); }
void pushClip(const Rect& r) {
    Rect c = gClip.empty() ? r : gClip.back().intersect(r);
    gClip.push_back(c);
    applyClip();
}
void popClip() {
    if (!gClip.empty()) gClip.pop_back();
    applyClip();
}

// ------------------------------------------------------------------ icons
void icon(Icon i, const Rect& r, const Color& c) {
    Vec2 m = r.center();
    float s = std::min(r.w, r.h) * 0.5f;
    switch (i) {
        case Icon::Play:
            triangle({m.x - s * 0.55f, m.y - s * 0.7f}, {m.x + s * 0.75f, m.y}, {m.x - s * 0.55f, m.y + s * 0.7f}, c);
            break;
        case Icon::Pause:
            roundRect(Rect(m.x - s * 0.6f, m.y - s * 0.7f, s * 0.42f, s * 1.4f), 1.5f, c);
            roundRect(Rect(m.x + s * 0.18f, m.y - s * 0.7f, s * 0.42f, s * 1.4f), 1.5f, c);
            break;
        case Icon::Step:
            triangle({m.x - s * 0.7f, m.y - s * 0.65f}, {m.x + s * 0.35f, m.y}, {m.x - s * 0.7f, m.y + s * 0.65f}, c);
            roundRect(Rect(m.x + s * 0.4f, m.y - s * 0.65f, s * 0.3f, s * 1.3f), 1, c);
            break;
        case Icon::Back:
            triangle({m.x + s * 0.7f, m.y - s * 0.65f}, {m.x - s * 0.35f, m.y}, {m.x + s * 0.7f, m.y + s * 0.65f}, c);
            roundRect(Rect(m.x - s * 0.7f, m.y - s * 0.65f, s * 0.3f, s * 1.3f), 1, c);
            break;
        case Icon::Finish:
            triangle({m.x - s * 0.8f, m.y - s * 0.6f}, {m.x, m.y}, {m.x - s * 0.8f, m.y + s * 0.6f}, c);
            triangle({m.x - s * 0.05f, m.y - s * 0.6f}, {m.x + s * 0.75f, m.y}, {m.x - s * 0.05f, m.y + s * 0.6f}, c);
            break;
        case Icon::Reset: {
            std::vector<Vec2> pts;
            for (int k = 0; k <= 24; ++k) {
                float a = float(M_PI) * (0.25f + 1.55f * k / 24.0f);
                pts.push_back({m.x + std::cos(a) * s * 0.62f, m.y - std::sin(a) * s * 0.62f});
            }
            polyline(pts, c, std::max(1.6f, s * 0.2f));
            Vec2 tip = pts.front();
            arrowHead(tip + Vec2(s * 0.12f, s * 0.12f), Vec2(1, 1), s * 0.5f, c);
            break;
        }
        case Icon::Chip: {
            Rect body(m.x - s * 0.55f, m.y - s * 0.55f, s * 1.1f, s * 1.1f);
            for (int k = 0; k < 3; ++k) {
                float o = -s * 0.32f + k * s * 0.32f;
                line({m.x + o, m.y - s * 0.85f}, {m.x + o, m.y - s * 0.55f}, c, 1.6f);
                line({m.x + o, m.y + s * 0.55f}, {m.x + o, m.y + s * 0.85f}, c, 1.6f);
                line({m.x - s * 0.85f, m.y + o}, {m.x - s * 0.55f, m.y + o}, c, 1.6f);
                line({m.x + s * 0.55f, m.y + o}, {m.x + s * 0.85f, m.y + o}, c, 1.6f);
            }
            roundRectOutline(body, s * 0.18f, c, 1.8f);
            roundRect(body.inset(s * 0.28f), 2, c);
            break;
        }
        case Icon::Bolt:
            convexPolygon({{m.x + s * 0.15f, m.y - s * 0.85f}, {m.x - s * 0.5f, m.y + s * 0.1f}, {m.x, m.y + s * 0.1f}}, c);
            convexPolygon({{m.x, m.y - s * 0.1f}, {m.x + s * 0.5f, m.y - s * 0.1f}, {m.x - s * 0.15f, m.y + s * 0.85f}}, c);
            break;
        case Icon::Layers:
            for (int k = 0; k < 3; ++k) {
                float y = m.y - s * 0.5f + k * s * 0.42f;
                convexPolygon({{m.x, y - s * 0.28f}, {m.x + s * 0.75f, y}, {m.x, y + s * 0.28f}, {m.x - s * 0.75f, y}},
                              c.alpha(k == 0 ? 1.f : 0.55f));
            }
            break;
        case Icon::Grid:
            for (int a = 0; a < 2; ++a)
                for (int b = 0; b < 2; ++b)
                    roundRect(Rect(m.x - s * 0.7f + a * s * 0.75f, m.y - s * 0.7f + b * s * 0.75f, s * 0.62f, s * 0.62f), 2, c);
            break;
        case Icon::Cube: {
            Vec2 t{m.x, m.y - s * 0.8f}, l{m.x - s * 0.7f, m.y - s * 0.4f}, rr{m.x + s * 0.7f, m.y - s * 0.4f};
            Vec2 cc{m.x, m.y}, bl{m.x - s * 0.7f, m.y + s * 0.4f}, br{m.x + s * 0.7f, m.y + s * 0.4f}, b{m.x, m.y + s * 0.8f};
            convexPolygon({t, rr, cc, l}, c);
            convexPolygon({l, cc, b, bl}, c.alpha(0.7f));
            convexPolygon({cc, rr, br, b}, c.alpha(0.45f));
            break;
        }
        case Icon::Database:
            for (int k = 0; k < 3; ++k)
                roundRect(Rect(m.x - s * 0.65f, m.y - s * 0.75f + k * s * 0.52f, s * 1.3f, s * 0.4f), s * 0.2f,
                          c.alpha(1.f - k * 0.2f));
            break;
        case Icon::Code:
            polyline({{m.x - s * 0.25f, m.y - s * 0.55f}, {m.x - s * 0.75f, m.y}, {m.x - s * 0.25f, m.y + s * 0.55f}}, c, 2.0f);
            polyline({{m.x + s * 0.25f, m.y - s * 0.55f}, {m.x + s * 0.75f, m.y}, {m.x + s * 0.25f, m.y + s * 0.55f}}, c, 2.0f);
            break;
        case Icon::Check:
            polyline({{m.x - s * 0.6f, m.y}, {m.x - s * 0.15f, m.y + s * 0.45f}, {m.x + s * 0.65f, m.y - s * 0.5f}}, c, 2.4f);
            break;
        case Icon::Cross:
            line({m.x - s * 0.5f, m.y - s * 0.5f}, {m.x + s * 0.5f, m.y + s * 0.5f}, c, 2.2f);
            line({m.x + s * 0.5f, m.y - s * 0.5f}, {m.x - s * 0.5f, m.y + s * 0.5f}, c, 2.2f);
            break;
        case Icon::Plus:
            line({m.x - s * 0.55f, m.y}, {m.x + s * 0.55f, m.y}, c, 2.0f);
            line({m.x, m.y - s * 0.55f}, {m.x, m.y + s * 0.55f}, c, 2.0f);
            break;
        case Icon::Minus:
            line({m.x - s * 0.55f, m.y}, {m.x + s * 0.55f, m.y}, c, 2.0f);
            break;
        case Icon::ChevronL:
            polyline({{m.x + s * 0.25f, m.y - s * 0.55f}, {m.x - s * 0.3f, m.y}, {m.x + s * 0.25f, m.y + s * 0.55f}}, c, 2.2f);
            break;
        case Icon::ChevronR:
            polyline({{m.x - s * 0.25f, m.y - s * 0.55f}, {m.x + s * 0.3f, m.y}, {m.x - s * 0.25f, m.y + s * 0.55f}}, c, 2.2f);
            break;
        case Icon::Save:
            roundRectOutline(Rect(m.x - s * 0.65f, m.y - s * 0.65f, s * 1.3f, s * 1.3f), 2, c, 1.8f);
            rect(Rect(m.x - s * 0.35f, m.y - s * 0.65f, s * 0.7f, s * 0.4f), c);
            rect(Rect(m.x - s * 0.4f, m.y + s * 0.1f, s * 0.8f, s * 0.45f), c.alpha(0.6f));
            break;
        case Icon::Folder:
            roundRect(Rect(m.x - s * 0.75f, m.y - s * 0.55f, s * 0.6f, s * 0.3f), 2, c);
            roundRect(Rect(m.x - s * 0.75f, m.y - s * 0.35f, s * 1.5f, s * 0.95f), 2, c);
            break;
        case Icon::Sun: {
            circle(m, s * 0.36f, c, 24);
            for (int k = 0; k < 8; ++k) {
                float a = float(M_PI) * k / 4;
                Vec2 d(std::cos(a), std::sin(a));
                line(m + d * (s * 0.55f), m + d * (s * 0.82f), c, 1.8f);
            }
            break;
        }
        case Icon::Moon: {
            // crescent: a disc with a second disc cut out (drawn in background-agnostic way)
            std::vector<Vec2> pts;
            for (int k = 0; k <= 24; ++k) {
                float a = float(M_PI) * (0.35f + 1.3f * k / 24.0f);
                pts.push_back({m.x + std::cos(a) * s * 0.7f, m.y + std::sin(a) * s * 0.7f});
            }
            for (int k = 24; k >= 0; --k) {
                float a = float(M_PI) * (0.55f + 0.9f * k / 24.0f);
                pts.push_back({m.x + s * 0.28f + std::cos(a) * s * 0.52f, m.y - s * 0.12f + std::sin(a) * s * 0.52f});
            }
            pts.push_back(pts.front());
            polyline(pts, c, 2.0f);
            break;
        }
        case Icon::Book:
            roundRectOutline(Rect(m.x - s * 0.7f, m.y - s * 0.6f, s * 0.66f, s * 1.2f), 2, c, 1.8f);
            roundRectOutline(Rect(m.x + s * 0.04f, m.y - s * 0.6f, s * 0.66f, s * 1.2f), 2, c, 1.8f);
            line({m.x - s * 0.5f, m.y - s * 0.25f}, {m.x - s * 0.2f, m.y - s * 0.25f}, c, 1.5f);
            line({m.x + s * 0.24f, m.y - s * 0.25f}, {m.x + s * 0.54f, m.y - s * 0.25f}, c, 1.5f);
            line({m.x - s * 0.5f, m.y + s * 0.05f}, {m.x - s * 0.2f, m.y + s * 0.05f}, c, 1.5f);
            line({m.x + s * 0.24f, m.y + s * 0.05f}, {m.x + s * 0.54f, m.y + s * 0.05f}, c, 1.5f);
            break;
        case Icon::Export:
            line({m.x, m.y + s * 0.3f}, {m.x, m.y - s * 0.7f}, c, 2.0f);
            arrowHead({m.x, m.y - s * 0.8f}, {0, -1}, s * 0.5f, c);
            polyline({{m.x - s * 0.7f, m.y}, {m.x - s * 0.7f, m.y + s * 0.7f}, {m.x + s * 0.7f, m.y + s * 0.7f},
                      {m.x + s * 0.7f, m.y}}, c, 2.0f);
            break;
    }
}

} // namespace vas::gfx::draw
