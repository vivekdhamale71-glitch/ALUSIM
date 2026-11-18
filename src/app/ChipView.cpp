// =====================================================================
//  3D chip view                                 [CG CO2, CO3, CO4, CO6]
//  The same datapath, built as a 3D processor die:
//   * every unit is an extruded solid; the ALU and MUX are extruded
//     from their 2D outlines (2D -> 3D objects)                  [CO3]
//   * perspective (gluPerspective) or parallel (glOrtho) projection
//     with an orbiting camera (gluLookAt)                        [CO4]
//   * rotation / translation / scaling of the model view          [CO2]
//   * a procedurally generated PCB texture, two lights with specular
//     highlights, emissive glow on the active unit, depth fog     [CO6]
//  Active units and buses follow the instruction being executed.
// =====================================================================
#include <cmath>
#include <cstdio>

#include "engine/gfx/GL.h"
#include "engine/gfx/Extrude.h"
#include "engine/gfx/Clipper.h"
#include "engine/gfx/Lighting.h"
#include "engine/gfx/Texture.h"
#include "app/DatapathLayout.h"
#include "app/Views.h"

namespace vas::ui {

using namespace gfx;
using namespace core;
namespace D = gfx::draw;

namespace {

constexpr float SX = 18.0f / layout::WORLD_W;   // world units per layout pixel
constexpr float SZ = 10.5f / layout::WORLD_H;

Vec2 toBoard(Vec2 p) { return {(p.x - layout::WORLD_W / 2) * SX, (p.y - layout::WORLD_H / 2) * SZ}; }

float unitHeight(int u) {
    switch (u) {
        case U_ALU: return 1.25f;
        case U_REGS: return 0.95f;
        case U_DMEM: return 1.05f;
        case U_IMEM: return 0.95f;
        case U_CU: return 0.85f;
        case U_MUX: case U_WB: return 0.5f;
        default: return 0.55f;
    }
}

Color unitColor(int u) {
    switch (u) {
        case U_ALU: return Color::hex(0x2B3F6B);
        case U_CU: return Color::hex(0x31285A);
        case U_REGS: return Color::hex(0x1E3A5A);
        case U_DMEM: return Color::hex(0x3A2348);
        case U_IMEM: return Color::hex(0x1F3550);
        default: return Color::hex(0x243452);
    }
}

// text lying flat on the XZ plane at height y
void flatText(const std::string& s, float x, float y, float z, float scale, Face f, int px, const Color& c, bool centered) {
    Font& font = Fonts::get(f, px);
    float w = font.width(s);
    glPushMatrix();
    glTranslatef(x, y, z);
    glRotatef(90, 1, 0, 0);          // font +y (down the page) -> world +z
    glScalef(scale, scale, scale);
    font.draw(centered ? -w / 2 : 0, -font.height() / 2, s, c);
    glPopMatrix();
}

} // namespace

void ChipView::initTextures() { boardTex_ = makePcbTexture(); }   // CG CO6 Texture.cpp

void ChipView::drawChip(const Rect& vp, Sim& sim, UI& ui) {
    if (!boardTex_) initTextures();

    // camera input
    if (ui.hovered(vp) && ui.in.pressed) { dragging_ = true; last_ = ui.in.mouse; }
    if (!ui.in.down) dragging_ = false;
    if (dragging_) {
        Vec2 d = ui.in.mouse - last_;
        yaw_ += d.x * 0.35f;
        pitch_ = clampf(pitch_ + d.y * 0.3f, 12, 85);
        last_ = ui.in.mouse;
    }
    if (ui.hovered(vp) && ui.in.wheel != 0) dist_ = clampf(dist_ * (ui.in.wheel > 0 ? 0.9f : 1.1f), 9, 45);
    if (autoRotate_ && !dragging_ && flat_ < 0.01f) yaw_ += ui.dt * 6;
    pulse_ += ui.dt;

    const MicroStep* ms = sim.currentStep();
    bool live = ms && sim.animating();
    Color pcol = live ? theme::phaseColor(ms->phase) : theme::cyan;
    for (int u = 0; u < UNIT_COUNT; ++u)
        glow_[u] = approach(glow_[u], live && ((ms->units >> u) & 1) ? 1.f : 0.f, 8, ui.dt);

    // ---- 3D pass ---------------------------------------------------------
    int H = D::screenH();
    glViewport(int(vp.x), int(H - vp.b()), int(vp.w), int(vp.h));
    glEnable(GL_SCISSOR_TEST);
    glScissor(int(vp.x), int(H - vp.b()), int(vp.w), int(vp.h));
    clipper::setWindow(nullptr);   // 3D: clipped by the view volume and this viewport
    glClearColor(0.03f, 0.05f, 0.1f, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // backdrop gradient
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, 1, 0, 1, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glDisable(GL_DEPTH_TEST);
    glBegin(GL_QUADS);
    if (theme::light) { glColor3f(0.80f, 0.85f, 0.93f); glVertex2f(0, 0); glVertex2f(1, 0);
                        glColor3f(0.95f, 0.97f, 1.0f); glVertex2f(1, 1); glVertex2f(0, 1); }
    else { glColor3f(0.02f, 0.04f, 0.09f); glVertex2f(0, 0); glVertex2f(1, 0);
           glColor3f(0.07f, 0.11f, 0.22f); glVertex2f(1, 1); glVertex2f(0, 1); }
    glEnd();

    // 2D mode looks straight down on the die (top view)
    applyCamera(vp, yaw_, pitch_, dist_ * (1 + 0.22f * easeInOutCubic(flat_)), 0, 89.5f, 0, 0.3f, 0);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    chipLights(pcol);                                              // CG CO6 Lighting.cpp
    Color fogc = theme::light ? Color(0.88f, 0.91f, 0.97f) : Color(0.03f, 0.05f, 0.1f);
    depthFog(flat_ < 0.02f, fogc, dist_ * 0.9f, dist_ * 2.6f);

    // package + die (textured board)
    material(Color::hex(0x0E1526), Color(0, 0, 0));
    box(-10.2f, -6.4f, 10.2f, 6.4f, -0.55f, -0.05f);
    material(Color(0.9f, 0.95f, 1.0f), Color(0, 0, 0), 20);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, boardTex_);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glBegin(GL_QUADS);
    glNormal3f(0, 1, 0);
    glTexCoord2f(0, 0); glVertex3f(-9.6f, 0, -5.8f);
    glTexCoord2f(0, 2); glVertex3f(-9.6f, 0, 5.8f);
    glTexCoord2f(3.3f, 2); glVertex3f(9.6f, 0, 5.8f);
    glTexCoord2f(3.3f, 0); glVertex3f(9.6f, 0, -5.8f);
    glEnd();
    glDisable(GL_TEXTURE_2D);
    // pins (QFP style) along the package edge
    material(Color::hex(0xC9A35A), Color(0.05f, 0.04f, 0.0f), 80);
    for (int i = 0; i < 28; ++i) {
        float x = -9.2f + i * (18.4f / 27);
        box(x - 0.14f, -7.2f, x + 0.14f, -6.4f, -0.45f, -0.3f);
        box(x - 0.14f, 6.4f, x + 0.14f, 7.2f, -0.45f, -0.3f);
    }
    for (int i = 0; i < 16; ++i) {
        float z = -5.6f + i * (11.2f / 15);
        box(-11.0f, z - 0.14f, -10.2f, z + 0.14f, -0.45f, -0.3f);
        box(10.2f, z - 0.14f, 11.0f, z + 0.14f, -0.45f, -0.3f);
    }

    // bus traces
    glDisable(GL_LIGHTING);
    std::vector<char> active(BUS_COUNT, 0);
    if (live) for (int b : ms->buses) active[b] = 1;
    for (int b = 0; b < BUS_COUNT; ++b) {
        auto path = layout::busPath(b);
        for (std::size_t i = 0; i + 1 < path.size(); ++i) {
            Vec2 a = toBoard(path[i]), e = toBoard(path[i + 1]);
            Vec2 d = (e - a).normalized().perp() * (active[b] ? 0.07f : 0.04f);
            Color col = active[b] ? pcol : Color::hex(0x3B5C8C);
            glColor4f(col.r, col.g, col.b, active[b] ? 1.f : 0.8f);
            glBegin(GL_QUADS);
            glVertex3f(a.x + d.x, 0.012f, a.y + d.y); glVertex3f(e.x + d.x, 0.012f, e.y + d.y);
            glVertex3f(e.x - d.x, 0.012f, e.y - d.y); glVertex3f(a.x - d.x, 0.012f, a.y - d.y);
            glEnd();
        }
    }

    // units
    glEnable(GL_LIGHTING);
    for (const auto& blk : layout::blocks()) {
        int u = blk.unit;
        float g = glow_[u];
        Color em = pcol * (0.55f * g);
        material(unitColor(u).lerp(pcol, 0.25f * g), Color(em.r, em.g, em.b), 60);
        float h = unitHeight(u) + 0.15f * g * std::sin(pulse_ * 6) * 0.3f;
        if (u == U_ALU || u == U_MUX || u == U_WB) {
            auto shape = u == U_ALU ? layout::aluShape(blk.r) : layout::muxShape(blk.r);
            std::vector<Vec2> poly;
            for (auto p : shape) poly.push_back(toBoard(p));
            // reverse so the winding is counter-clockwise seen from above
            std::vector<Vec2> ccw(poly.rbegin(), poly.rend());
            Vec2 fc = toBoard({blk.r.x + blk.r.w * 0.62f, blk.r.y + blk.r.h * 0.5f});
            prism(ccw, h, fc);
        } else {
            Vec2 p0 = toBoard({blk.r.x, blk.r.y}), p1 = toBoard({blk.r.r(), blk.r.b()});
            box(p0.x, p0.y, p1.x, p1.y, 0, h);
        }
    }

    // glow halos + bus pulses (additive, unlit)
    glDisable(GL_LIGHTING);
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    for (const auto& blk : layout::blocks()) {
        float g = glow_[blk.unit];
        if (g < 0.02f) continue;
        Vec2 p0 = toBoard({blk.r.x, blk.r.y}), p1 = toBoard({blk.r.r(), blk.r.b()});
        for (int k = 1; k <= 6; ++k) {
            float e = 0.12f * k;
            glColor4f(pcol.r, pcol.g, pcol.b, g * 0.12f);
            glBegin(GL_QUADS);
            glVertex3f(p0.x - e, 0.02f, p0.y - e); glVertex3f(p1.x + e, 0.02f, p0.y - e);
            glVertex3f(p1.x + e, 0.02f, p1.y + e); glVertex3f(p0.x - e, 0.02f, p1.y + e);
            glEnd();
        }
    }
    if (live) {
        float t = sim.stepT;
        for (int b : ms->buses) {
            auto path = layout::busPath(b);
            if (path.size() < 2) continue;
            float L = 0;
            for (std::size_t i = 0; i + 1 < path.size(); ++i) L += (path[i + 1] - path[i]).length();
            for (int k = 0; k < 3; ++k) {
                float d = std::fmod(t + k / 3.f, 1.f) * L;
                Vec2 p = path.front();
                for (std::size_t i = 0; i + 1 < path.size(); ++i) {
                    float s = (path[i + 1] - path[i]).length();
                    if (d <= s) { p = path[i] + (path[i + 1] - path[i]) * (s > 0 ? d / s : 0); break; }
                    d -= s;
                }
                Vec2 q = toBoard(p);
                for (int r = 3; r >= 1; --r) {
                    float rad = 0.07f * r;
                    glColor4f(1, 1, 1, 0.22f);
                    glBegin(GL_TRIANGLE_FAN);
                    glVertex3f(q.x, 0.05f, q.y);
                    for (int a = 0; a <= 16; ++a) {
                        float ang = 6.2831853f * a / 16;
                        glVertex3f(q.x + std::cos(ang) * rad, 0.05f, q.y + std::sin(ang) * rad);
                    }
                    glEnd();
                }
            }
        }
    }
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // labels printed on the top faces (texture-mapped glyph quads)      [CG CO6]
    glDisable(GL_FOG);
    for (const auto& blk : layout::blocks()) {
        int u = blk.unit;
        Vec2 cpos = toBoard(blk.r.center());
        if (u == U_ALU) cpos = toBoard({blk.r.x + blk.r.w * 0.62f, blk.r.center().y});
        float h = unitHeight(u) + 0.01f;
        const char* names[UNIT_COUNT] = {"PC", "IMEM", "IR", "CONTROL", "REGISTERS", "IMM", "M", "ALU", "FLAGS", "DMEM", "WB"};
        float scale = (u == U_MUX || u == U_WB) ? 0.012f : 0.02f;
        Color col = theme::text.lerp(pcol.lighten(0.5f), glow_[u]);
        flatText(names[u], cpos.x, h, cpos.y, scale, Face::Bold, 32, col, true);
        if (u == U_ALU && sim.hasCurrent) {
            std::string sym = aluOpSymbol(sim.current.aluOp);
            if (!sym.empty()) flatText(sym, cpos.x, h, cpos.y + 1.0f, 0.03f, Face::Bold, 48, theme::amber, true);
        }
        if (u == U_REGS && sim.hasCurrent && !sim.current.regsWritten.empty()) {
            int r = sim.current.regsWritten.back();
            flatText(std::string(regName(r)) + " = " + fmtValue(std::int64_t(sim.shownRegs[r])), cpos.x, h, cpos.y + 0.9f,
                     0.012f, Face::MonoBold, 32, theme::amber, true);
        }
    }
    flatText("VIRTUAL ALU  x86-64", -9.2f, 0.02f, 5.2f, 0.016f, Face::Bold, 32, theme::textMute, false);
    glDepthMask(GL_TRUE);

    // restore 2D state
    glDisable(GL_LIGHTING);
    glDisable(GL_LIGHT0);
    glDisable(GL_LIGHT1);
    glDisable(GL_FOG);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_NORMALIZE);
    glDisable(GL_SCISSOR_TEST);
    D::beginFrame(D::screenW(), D::screenH());

    // ---- overlays ----------------------------------------------------------
    ui.toggle("autorot", Rect(vp.x + 16, vp.y + 64, 150, 28), autoRotate_, "Auto-rotate");
    if (ui.button("camreset", Rect(vp.x + 176, vp.y + 61, 110, 34), "Reset view", Style::Secondary, int(Icon::Reset))) {
        yaw_ = -32; pitch_ = 52; dist_ = 21;
    }
    // current activity
    Rect info(vp.x + 16, vp.b() - 64, std::min(520.f, vp.w - 32), 50);
    D::roundRect(info, 12, Color::hex(0x0B1222, 0.85f));
    D::roundRectOutline(info, 12, pcol.alpha(0.5f));
    if (sim.hasCurrent) {
        drawAsmLine(info.x + 14, info.y + 7, sim.current.text, 1.f, 14);
        std::string sub = ms ? std::string(phaseName(ms->phase)) + "  \xC2\xB7  " + ms->microOp : "";
        D::text(Face::Mono, 11, info.x + 14, info.y + 28, D::ellipsize(Face::Mono, 11, sub, info.w - 28), pcol.lighten(0.3f));
    } else {
        D::textIn(Face::Regular, 13, info, "Run a program to watch data move across the die", theme::textDim, Align::Left, 14);
    }
    D::text(Face::Regular, 11, vp.r() - 16, vp.b() - 24,
            "drag to orbit  \xC2\xB7  scroll to zoom  \xC2\xB7  lit, fogged, texture-mapped OpenGL scene", theme::textMute,
            Align::Right);
}

} // namespace vas::ui
