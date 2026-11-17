#include "app/UI.h"

#include <algorithm>
#include <cmath>

namespace vas::ui {

using namespace gfx;
namespace D = gfx::draw;

UI& UI::get() {
    static UI ui;
    return ui;
}

std::vector<std::string> splitLines(const std::string& s) {
    std::vector<std::string> out(1);
    for (char c : s) {
        if (c == '\n') out.emplace_back();
        else out.back() += c;
    }
    return out;
}

void UI::beginFrame(float d) {
    dt = d;
    time += d;
    keyboardTaken = false;
    tip_.clear();
    mouseBlocked_ = false;
}

void UI::endFrame() {
    if (!tip_.empty()) {
        float w = D::textWidth(Face::Regular, 12, tip_) + 18;
        Vec2 p = tipPos_ + Vec2(14, 18);
        if (p.x + w > D::screenW() - 6) p.x = D::screenW() - 6 - w;
        Rect r(p.x, p.y, w, 26);
        D::shadow(r, 7, 10, Color(0, 0, 0, 1));
        D::roundRect(r, 7, theme::panel3);
        D::roundRectOutline(r, 7, theme::borderHi);
        D::textIn(Face::Regular, 12, r, tip_, theme::text, Align::Center);
    }
    if (in.released) pressedId_.clear();
    in.pressed = in.released = false;
    in.wheel = 0;
    in.keys.clear();
}

float UI::anim(const std::string& id, float target, float speed) {
    auto it = anims_.find(id);
    if (it == anims_.end()) { anims_[id] = target; return target; }
    it->second = approach(it->second, target, speed, dt);
    return it->second;
}

void UI::tooltip(const std::string& s) {
    tip_ = s;
    tipPos_ = in.mouse;
}

bool UI::button(const std::string& id, const Rect& r, const std::string& label, Style st, int icon, bool enabled) {
    bool hov = enabled && hovered(r);
    if (hov && in.pressed) pressedId_ = id;
    float h = anim(id + "#h", hov ? 1.f : 0.f);
    float p = anim(id + "#p", (hov && in.down && pressedId_ == id) ? 1.f : 0.f, 25);
    Rect rr = r;
    float shrink = p * 1.2f;
    rr = rr.inset(shrink);
    float a = enabled ? 1.f : 0.38f;
    Color fg = theme::text;
    float rad = std::min(10.f, r.h / 2);
    switch (st) {
        case Style::Primary: {
            if (h > 0.01f) D::glow(rr, rad, 10 * h, theme::cyan.alpha(a));
            Color t = Color::hex(0x4CC9FF).lighten(0.12f * h), b = Color::hex(0x2F6BFF).lighten(0.12f * h);
            D::roundRectGrad(rr, rad, t.alpha(a), b.alpha(a));
            D::roundRectOutline(rr, rad, Color(1, 1, 1, 0.18f * a));
            fg = theme::onAccent;
            break;
        }
        case Style::Secondary:
            D::roundRectGrad(rr, rad, theme::panel3.lighten(0.05f * h).alpha(a), theme::panel2.lighten(0.04f * h).alpha(a));
            D::roundRectOutline(rr, rad, theme::borderHi.lerp(theme::cyan, 0.35f * h).alpha(a));
            break;
        case Style::Ghost:
            if (h > 0.01f) D::roundRect(rr, rad, theme::panel3.alpha(h * a));
            fg = theme::textDim.lerp(theme::text, h);
            break;
        case Style::Danger:
            D::roundRect(rr, rad, theme::red.alpha((0.16f + 0.14f * h) * a));
            D::roundRectOutline(rr, rad, theme::red.alpha(0.5f * a));
            fg = (theme::light ? theme::red : Color::hex(0xFFD5D5));
            break;
        case Style::Success:
            D::roundRect(rr, rad, theme::green.alpha((0.16f + 0.14f * h) * a));
            D::roundRectOutline(rr, rad, theme::green.alpha(0.5f * a));
            fg = (theme::light ? theme::green : Color::hex(0xD5FFEA));
            break;
    }
    fg = fg.alpha(a);
    Face face = st == Style::Primary ? Face::Bold : Face::Semibold;
    int px = r.h >= 34 ? 14 : 13;
    float tw = D::textWidth(face, px, label);
    float iw = icon >= 0 ? 16.f : 0.f;
    float gapw = (icon >= 0 && !label.empty()) ? 8.f : 0.f;
    float x = rr.x + (rr.w - tw - iw - gapw) / 2;
    if (icon >= 0) D::icon(static_cast<Icon>(icon), Rect(x, rr.y + (rr.h - 16) / 2, 16, 16), fg);
    D::textIn(face, px, Rect(x + iw + gapw, rr.y, tw + 2, rr.h), label, fg);
    return enabled && clicked(r) && pressedId_ == id;
}

bool UI::iconButton(const std::string& id, const Rect& r, Icon icon, const std::string& tip, Style st, bool enabled) {
    bool res = button(id, r, "", st, int(icon), enabled);
    if (hovered(r) && !tip.empty()) tooltip(tip);
    return res;
}

int UI::segmented(const std::string& id, const Rect& r, const std::vector<std::string>& opts, int cur,
                  const Color& accent) {
    D::roundRect(r, std::min(10.f, r.h / 2), theme::bg1);
    D::roundRectOutline(r, std::min(10.f, r.h / 2), theme::border);
    if (opts.empty()) return cur;
    float w = (r.w - 6) / opts.size();
    float x = anim(id + "#x", r.x + 3 + w * cur, 16);
    Rect sel(x, r.y + 3, w, r.h - 6);
    D::roundRectGrad(sel, std::min(8.f, sel.h / 2), accent.alpha(0.30f), accent.alpha(0.18f));
    D::roundRectOutline(sel, std::min(8.f, sel.h / 2), accent.alpha(0.55f));
    int res = cur;
    for (std::size_t i = 0; i < opts.size(); ++i) {
        Rect cell(r.x + 3 + w * i, r.y, w, r.h);
        bool hov = hovered(cell);
        if (hov && in.pressed) pressedId_ = id + std::to_string(i);
        float h = anim(id + "#h" + std::to_string(i), hov ? 1.f : 0.f);
        Color c = int(i) == cur ? theme::text : theme::textDim.lerp(theme::text, h * 0.6f);
        D::textIn(int(i) == cur ? Face::Bold : Face::Semibold, r.h > 30 ? 13 : 12, cell, opts[i], c, Align::Center);
        if (clicked(cell) && pressedId_ == id + std::to_string(i)) res = int(i);
    }
    return res;
}

bool UI::toggle(const std::string& id, const Rect& r, bool& v, const std::string& label) {
    bool hov = hovered(r);
    if (hov && in.pressed) pressedId_ = id;
    bool changed = false;
    if (clicked(r) && pressedId_ == id) { v = !v; changed = true; }
    float t = anim(id + "#t", v ? 1.f : 0.f, 16);
    float h = anim(id + "#h", hov ? 1.f : 0.f);
    Rect track(r.x, r.y + (r.h - 20) / 2, 38, 20);
    D::roundRect(track, 10, theme::bg1.lerp(theme::green, 0.0f));
    D::roundRect(track, 10, theme::green.alpha(0.85f * t));
    D::roundRectOutline(track, 10, theme::borderHi.lerp(theme::green, t).alpha(0.8f + 0.2f * h));
    Vec2 knob(track.x + 10 + 18 * t, track.center().y);
    D::circle(knob + Vec2(0, 1), 8, Color(0, 0, 0, 0.3f));
    D::circle(knob, 7.5f, Color::hex(0xF8FAFC));
    D::textIn(Face::Semibold, 13, Rect(track.r() + 10, r.y, r.w - 48, r.h), label, theme::text.alpha(0.85f + 0.15f * h));
    return changed;
}

bool UI::slider(const std::string& id, const Rect& r, float& v, float lo, float hi) {
    bool hov = hovered(r);
    if (hov && in.pressed) pressedId_ = id;
    bool changed = false;
    if (pressedId_ == id && in.down) {
        float t = clampf((in.mouse.x - r.x) / r.w, 0, 1);
        float nv = lo + t * (hi - lo);
        changed = nv != v;
        v = nv;
    }
    float t = clampf((v - lo) / (hi - lo), 0, 1);
    float h = anim(id + "#h", (hov || pressedId_ == id) ? 1.f : 0.f);
    Rect track(r.x, r.center().y - 3, r.w, 6);
    D::roundRect(track, 3, theme::bg1);
    D::roundRectGrad(Rect(track.x, track.y, track.w * t, track.h), 3, theme::cyan, theme::blue);
    Vec2 k(r.x + r.w * t, r.center().y);
    if (h > 0.01f) D::circle(k, 12 * h, theme::cyan.alpha(0.18f));
    D::circle(k, 7, Color::hex(0xF8FAFC));
    D::ring(k, 7, 2, theme::cyan);
    return changed;
}

bool UI::stepper(const std::string& id, const Rect& r, int& v, int lo, int hi, const std::string& label) {
    bool changed = false;
    float lw = label.empty() ? 0 : D::textWidth(Face::Semibold, 12, label) + 10;
    if (!label.empty()) D::textIn(Face::Semibold, 12, Rect(r.x, r.y, lw, r.h), label, theme::textDim);
    Rect box(r.x + lw, r.y, r.w - lw, r.h);
    D::roundRect(box, 8, theme::bg1);
    D::roundRectOutline(box, 8, theme::border);
    Rect minus(box.x + 2, box.y + 2, box.h - 4, box.h - 4), plus(box.r() - box.h + 2, box.y + 2, box.h - 4, box.h - 4);
    if (iconButton(id + "-", minus, Icon::Minus, "", Style::Ghost, v > lo)) { --v; changed = true; }
    if (iconButton(id + "+", plus, Icon::Plus, "", Style::Ghost, v < hi)) { ++v; changed = true; }
    if (hovered(box) && in.wheel != 0) {
        v = std::max(lo, std::min(hi, v + (in.wheel > 0 ? 1 : -1)));
        changed = true;
    }
    D::textIn(Face::MonoBold, 14, box, std::to_string(v), theme::text, Align::Center);
    return changed;
}

Rect UI::card(const Rect& r, const std::string& title, const Color& accent, const std::string& right) {
    D::shadow(r, theme::radius, 18, Color(0, 0, 0, 1));
    D::roundRectGrad(r, theme::radius, theme::panel.lighten(0.025f), theme::panel);
    D::roundRectOutline(r, theme::radius, theme::border);
    // subtle top highlight
    D::rectGradH(Rect(r.x + 18, r.y + 0.5f, r.w - 36, 1), Color(1, 1, 1, 0), Color(1, 1, 1, 0.06f));
    if (title.empty()) return r.inset(14);
    D::circle(Vec2(r.x + 20, r.y + 21), 3.5f, accent);
    D::circle(Vec2(r.x + 20, r.y + 21), 7, accent.alpha(0.18f));
    D::text(Face::Bold, 12, r.x + 32, r.y + 13, title, theme::textDim);
    if (!right.empty()) D::text(Face::Semibold, 12, r.r() - 16, r.y + 13, right, theme::textMute, Align::Right);
    return Rect(r.x + 14, r.y + 40, r.w - 28, r.h - 54);
}

void UI::pill(const Rect& r, const std::string& s, const Color& c, bool filled, int px) {
    float rad = r.h / 2;
    if (filled) {
        D::roundRectGrad(r, rad, c.lighten(0.1f), c);
        D::textIn(Face::Bold, px, r, s, theme::onAccent, Align::Center);
    } else {
        D::roundRect(r, rad, c.alpha(0.14f));
        D::roundRectOutline(r, rad, c.alpha(0.55f));
        D::textIn(Face::Semibold, px, r, s, c.lighten(0.35f), Align::Center);
    }
}

float UI::chip(float x, float y, const std::string& s, const Color& c, bool on, int px) {
    float w = D::textWidth(Face::MonoBold, px, s) + 18;
    Rect r(x, y, w, px + 12);
    if (on) {
        D::glow(r, r.h / 2, 6, c);
        D::roundRectGrad(r, r.h / 2, c.lighten(0.15f), c);
        D::textIn(Face::MonoBold, px, r, s, theme::onAccent, Align::Center);
    } else {
        D::roundRect(r, r.h / 2, theme::bg1);
        D::roundRectOutline(r, r.h / 2, theme::border);
        D::textIn(Face::Mono, px, r, s, theme::textMute, Align::Center);
    }
    return w;
}

void UI::emptyState(const Rect& r, const std::string& title, const std::string& body) {
    Vec2 c = r.center();
    D::circle(c - Vec2(0, 34), 26, theme::cyan.alpha(0.08f));
    D::icon(Icon::Chip, Rect(c.x - 16, c.y - 50, 32, 32), theme::cyan.alpha(0.8f));
    D::text(Face::Bold, 16, c.x, c.y - 4, title, theme::text, Align::Center);
    float w = std::min(r.w - 40, 420.f);
    D::paragraph(Face::Regular, 13, Rect(c.x - w / 2, c.y + 22, w, 80), body, theme::textDim);
}

// --------------------------------------------------------------- editor
static int lineStart(const std::string& t, int line) {
    int l = 0;
    for (int i = 0; i < int(t.size()); ++i) {
        if (l == line) return i;
        if (t[i] == '\n') ++l;
    }
    return l == line ? int(t.size()) : int(t.size());
}
static void cursorLineCol(const std::string& t, int cur, int& line, int& col) {
    line = 0;
    col = 0;
    for (int i = 0; i < cur && i < int(t.size()); ++i) {
        if (t[i] == '\n') { ++line; col = 0; } else ++col;
    }
}

bool UI::codeEditor(const std::string& id, const Rect& r, std::string& text, EditorState& st, const Highlighter& hl,
                    int activeLine, int errorLine) {
    (void)id;
    bool changed = false;
    const int px = 15;
    Font& mono = Fonts::get(Face::Mono, px);
    float cw = mono.width("M");
    float lh = std::round(mono.height() * 1.55f);
    float gutter = 46;

    bool hov = hovered(r);
    if (in.pressed) {
        st.focused = hov;
        if (hov) {
            // place the cursor where the user clicked
            std::vector<std::string> lines = splitLines(text);
            int line = int((in.mouse.y - r.y - 10 + st.scroll) / lh);
            line = std::max(0, std::min(int(lines.size()) - 1, line));
            int col = int(std::round((in.mouse.x - r.x - gutter - 10) / cw));
            col = std::max(0, std::min(int(lines[line].size()), col));
            st.cursor = lineStart(text, line) + col;
            st.blink = 0;
            st.preferredCol = -1;
        }
    }
    if (hov && in.wheel != 0) st.scroll -= in.wheel * lh * 3;

    if (st.focused) {
        for (int k : in.keys) {
            int line, col;
            cursorLineCol(text, st.cursor, line, col);
            std::vector<std::string> lines = splitLines(text);
            if (k >= 32 && k < 127) {
                text.insert(text.begin() + st.cursor, char(k));
                ++st.cursor;
                changed = true;
            } else if (k == 13) {
                std::string indent;
                for (char c : lines[line]) { if (c == ' ') indent += ' '; else break; }
                if (!lines[line].empty() && lines[line].back() == '{') indent += "    ";
                text.insert(st.cursor, "\n" + indent);
                st.cursor += 1 + int(indent.size());
                changed = true;
            } else if (k == 9) {
                text.insert(st.cursor, "    ");
                st.cursor += 4;
                changed = true;
            } else if (k == 8) {
                if (st.cursor > 0) { text.erase(std::size_t(st.cursor - 1), 1); --st.cursor; changed = true; }
            } else if (k == 127 || k == K_DELETE) {
                if (st.cursor < int(text.size())) { text.erase(std::size_t(st.cursor), 1); changed = true; }
            } else if (k == K_LEFT) {
                st.cursor = std::max(0, st.cursor - 1);
                st.preferredCol = -1;
            } else if (k == K_RIGHT) {
                st.cursor = std::min(int(text.size()), st.cursor + 1);
                st.preferredCol = -1;
            } else if (k == K_UP || k == K_DOWN) {
                int target = line + (k == K_UP ? -1 : 1);
                if (st.preferredCol < 0) st.preferredCol = col;
                if (target >= 0 && target < int(lines.size()))
                    st.cursor = lineStart(text, target) + std::min(st.preferredCol, int(lines[target].size()));
            } else if (k == K_HOME) {
                st.cursor = lineStart(text, line);
            } else if (k == K_END) {
                st.cursor = lineStart(text, line) + int(lines[line].size());
            } else {
                continue;
            }
            st.blink = 0;
        }
        keyboardTaken = true;
    }
    st.blink += dt;

    // frame
    float f = anim(id + "#focus", st.focused ? 1.f : 0.f);
    D::roundRect(r, 12, theme::surface);
    D::roundRectOutline(r, 12, theme::border.lerp(theme::cyan, f * 0.7f), 1 + f * 0.5f);
    if (f > 0.01f) D::glow(r, 12, 6 * f, theme::cyan.alpha(0.35f));
    D::rect(Rect(r.x + gutter, r.y + 8, 1, r.h - 16), theme::border.alpha(0.7f));

    std::vector<std::string> lines = splitLines(text);
    float contentH = lines.size() * lh + 20;
    st.scroll = clampf(st.scroll, 0, std::max(0.f, contentH - r.h));
    int curLine, curCol;
    cursorLineCol(text, st.cursor, curLine, curCol);
    if (st.focused && !in.keys.empty()) {   // keep the cursor visible
        float cy = curLine * lh;
        if (cy - st.scroll < 0) st.scroll = cy;
        if (cy + lh - st.scroll > r.h - 20) st.scroll = cy + lh - r.h + 20;
    }

    D::pushClip(r.inset(1));
    float y0 = r.y + 10 - st.scroll;
    for (int i = 0; i < int(lines.size()); ++i) {
        float y = y0 + i * lh;
        if (y + lh < r.y || y > r.b()) continue;
        Rect lr(r.x + 2, y, r.w - 4, lh);
        if (i == activeLine) {
            D::rectGradH(lr, theme::amber.alpha(0.20f), theme::amber.alpha(0.02f));
            D::rect(Rect(r.x + 2, y, 3, lh), theme::amber);
        }
        if (i == errorLine) {
            D::rectGradH(lr, theme::red.alpha(0.22f), theme::red.alpha(0.03f));
            D::rect(Rect(r.x + 2, y, 3, lh), theme::red);
        }
        if (st.focused && i == curLine && i != activeLine && i != errorLine)
            D::rect(Rect(r.x + gutter + 1, y, r.w - gutter - 3, lh), Color(1, 1, 1, 0.03f));
        float ty = y + (lh - mono.height()) / 2;
        D::text(Face::Mono, 13, r.x + gutter - 10, ty + 1, std::to_string(i + 1),
                i == curLine && st.focused ? theme::textDim : theme::textMute, Align::Right);
        const std::string& ln = lines[i];
        std::vector<HighlightSpan> spans = hl ? hl(ln) : std::vector<HighlightSpan>{};
        float x = r.x + gutter + 10;
        int pos = 0;
        auto drawRun = [&](int a, int b, const Color& c) {
            if (b <= a) return;
            mono.draw(x + a * cw, ty, ln.substr(std::size_t(a), std::size_t(b - a)), c);
        };
        for (const auto& s : spans) {
            if (s.start > pos) drawRun(pos, s.start, theme::text);
            drawRun(s.start, s.end, s.color);
            pos = std::max(pos, s.end);
        }
        if (pos < int(ln.size())) drawRun(pos, int(ln.size()), theme::text);
        if (i == errorLine) {
            // squiggly underline (sine wave)
            std::vector<Vec2> wv;
            float w = std::max(cw * 3, cw * float(ln.size()));
            for (float k = 0; k <= w; k += 2) wv.push_back({x + k, ty + mono.height() + 2 + std::sin(k * 0.6f) * 1.5f});
            D::polyline(wv, theme::red, 1.2f);
        }
    }
    if (st.focused && std::fmod(st.blink, 1.0f) < 0.6f) {
        float cx = r.x + gutter + 10 + curCol * cw;
        float cy = y0 + curLine * lh + 3;
        D::rect(Rect(cx - 1, cy, 2, lh - 6), theme::cyan);
    }
    D::popClip();
    return changed;
}

} // namespace vas::ui
