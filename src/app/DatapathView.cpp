// =====================================================================
//  Datapath view                                  [COA CO1, CO4 | CG]
//  The processor datapath drawn in world coordinates and mapped to the
//  screen with a 2D transform (pan / zoom).                  [CG CO2]
//   * every wire is clipped to the viewport with Cohen-Sutherland [CO5]
//   * active wires are rasterised with Bresenham and animated as a
//     stream of signal dots                                    [CO3]
//   * the ALU / MUX symbols are filled with the scanline algorithm [CO3]
//  The control-signal vector of the current micro-step lights up the
//  control lines, and the waveform panel plots every signal over the
//  clock cycles (Liang-Barsky clipping while it scrolls).
// =====================================================================
#include <algorithm>
#include <cmath>
#include <cstdio>

#include "CG_2310218L/CO5_Clipping/Clip.h"
#include "CG_2310218L/CO1_Primitives/GL.h"
#include "CG_2310218L/CO3_ObjectAlgorithms/Raster.h"
#include "app/DatapathLayout.h"
#include "app/Views.h"

namespace vas::ui {

using namespace gfx;
using namespace core;
namespace D = gfx::draw;

namespace {

using layout::WORLD_W;
using layout::WORLD_H;
using layout::blockRect;
using layout::busPath;
using layout::aluShape;
using layout::muxShape;

const char* busLabel(int b) {
    switch (b) {
        case B_PC_IMEM: return "addr";
        case B_IMEM_IR: return "instr";
        case B_IR_CU: return "opcode";
        case B_IR_REGS: return "reg #";
        case B_IR_IMM: return "imm/disp";
        case B_REGA_ALU: return "A";
        case B_REGB_MUX: return "B";
        case B_MUX_ALU: return "B'";
        case B_ALU_DMEM: return "address";
        case B_REGB_DMEM: return "write data";
        case B_DMEM_WB: return "read data";
        case B_ALU_WB: return "result";
        case B_WB_REGS: return "write back";
        case B_FLAGS_CU: return "flags";
        case B_IR_PC: return "target";
        case B_DMEM_MUX: return "mem operand";
        default: return "";
    }
}

struct CtrlLine {
    int signal;            // index into ControlSignals, -1 = ALUOp
    const char* name;
    std::vector<Vec2> path;
};

const std::vector<CtrlLine>& controlLines() {
    static const std::vector<CtrlLine> l = {
        {0, "PCWrite", {{420, 34}, {88, 34}, {88, 240}}},
        {1, "IRWrite", {{420, 92}, {345, 92}, {345, 238}}},
        {4, "RegWrite", {{522, 102}, {522, 188}}},
        {5, "ALUSrc", {{612, 102}, {612, 272}, {636, 272}, {636, 290}}},
        {-1, "ALUOp", {{642, 72}, {742, 72}, {742, 210}}},
        {2, "MemRead", {{642, 48}, {890, 48}, {890, 196}}},
        {3, "MemWrite", {{642, 40}, {920, 40}, {920, 196}}},
        {6, "MemToReg", {{642, 28}, {1026, 28}, {1026, 250}}},
    };
    return l;
}

void fillScanline(const std::vector<Vec2>& poly, const Color& top, const Color& bottom, float y0, float y1) {
    // scanline polygon fill with a vertical gradient                  [CG CO3]
    auto spans = scanlineFill(poly);
    glBegin(GL_QUADS);
    for (const auto& s : spans) {
        float t = clampf((s.y - y0) / std::max(1.f, y1 - y0), 0, 1);
        Color c = top.lerp(bottom, t);
        glColor4f(c.r, c.g, c.b, c.a);
        glVertex2f(s.x0, float(s.y));
        glVertex2f(s.x1, float(s.y));
        glVertex2f(s.x1, float(s.y + 1));
        glVertex2f(s.x0, float(s.y + 1));
    }
    glEnd();
}

float pathLength(const std::vector<Vec2>& p) {
    float L = 0;
    for (std::size_t i = 0; i + 1 < p.size(); ++i) L += (p[i + 1] - p[i]).length();
    return L;
}
Vec2 pointAlong(const std::vector<Vec2>& p, float d) {
    for (std::size_t i = 0; i + 1 < p.size(); ++i) {
        float s = (p[i + 1] - p[i]).length();
        if (d <= s) return p[i] + (p[i + 1] - p[i]) * (s > 0 ? d / s : 0);
        d -= s;
    }
    return p.empty() ? Vec2() : p.back();
}

} // namespace

void DatapathView::draw(const Rect& area, Sim& sim, UI& ui) {
    Rect a = area;
    Rect wave = a.cutBottom(std::min(230.f, a.h * 0.3f));
    a.cutBottom(theme::gap);
    Rect info = a.cutRight(std::min(360.f, a.w * 0.3f));
    a.cutRight(theme::gap);
    drawDiagram(a, sim, ui);
    drawInfo(info, sim, ui);
    drawWaveform(wave, sim, ui);
}

void DatapathView::drawDiagram(const Rect& cardR, Sim& sim, UI& ui) {
    const MicroStep* ms = sim.currentStep();
    std::string right = ms ? std::string(phaseName(ms->phase)) : "idle";
    Rect c = ui.card(cardR, "DATAPATH", theme::blue, "");
    if (ui.hovered(Rect(cardR.x, cardR.y, 140, 40))) ui.tooltip("Drag to pan, scroll to zoom");
    Rect view = c.inset(-6, -4);

    // world -> screen transform (fit, then user zoom & pan)          [CG CO2]
    float fit = std::min(view.w / WORLD_W, view.h / WORLD_H);
    if (ui.hovered(view) && ui.in.wheel != 0) {
        Vec2 m = ui.in.mouse;
        float nz = clampf(zoom_ * (ui.in.wheel > 0 ? 1.12f : 1 / 1.12f), 0.6f, 3.5f);
        // zoom about the mouse position
        Vec2 worldCenter = view.center() + pan_;
        pan_ = pan_ + (m - worldCenter) * (1 - nz / zoom_);
        zoom_ = nz;
    }
    if (ui.hovered(view) && ui.in.pressed) { dragging_ = true; dragStart_ = ui.in.mouse; panStart_ = pan_; }
    if (!ui.in.down) dragging_ = false;
    if (dragging_) pan_ = panStart_ + (ui.in.mouse - dragStart_);
    float s = fit * zoom_;
    Mat3 M = Mat3::translate(view.center().x + pan_.x, view.center().y + pan_.y) * Mat3::scale(s, s) *
             Mat3::translate(-WORLD_W / 2, -WORLD_H / 2);
    auto W = [&](Vec2 p) { return M.apply(p); };
    auto WR = [&](const Rect& r) {
        Vec2 a0 = W({r.x, r.y}), a1 = W({r.r(), r.b()});
        return Rect(a0.x, a0.y, a1.x - a0.x, a1.y - a0.y);
    };
    int px = std::max(9, int(std::round(13 * s)));
    int pxs = std::max(8, int(std::round(11 * s)));

    // zoom buttons
    Rect zb(c.r() - 104, cardR.y + 8, 30, 26);
    if (ui.iconButton("dzm", zb, Icon::Minus, "Zoom out")) zoom_ = std::max(0.6f, zoom_ / 1.2f);
    if (ui.iconButton("dzp", Rect(zb.x + 34, zb.y, 30, 26), Icon::Plus, "Zoom in")) zoom_ = std::min(3.5f, zoom_ * 1.2f);
    if (ui.button("dzfit", Rect(zb.x + 68, zb.y, 36, 26), "fit", Style::Ghost)) { zoom_ = 1; pan_ = Vec2(); }
    if (ms) {
        Color pc = theme::phaseColor(ms->phase);
        float w = D::textWidth(Face::Bold, 11, right) + 20;
        ui.pill(Rect(zb.x - w - 10, cardR.y + 10, w, 22), right, pc, true, 11);
    }

    std::vector<char> activeBus(BUS_COUNT, 0);
    unsigned activeUnits = 0;
    Color pcol = theme::cyan;
    if (ms && sim.animating()) {
        for (int b : ms->buses) activeBus[b] = 1;
        activeUnits = ms->units;
        pcol = theme::phaseColor(ms->phase);
    }
    for (int u = 0; u < UNIT_COUNT; ++u) unitGlow_[u] = approach(unitGlow_[u], (activeUnits >> u) & 1 ? 1.f : 0.f, 10, ui.dt);

    D::pushClip(view);
    // ---- idle wires (clipped)                                      [CG CO5]
    auto drawPath = [&](const std::vector<Vec2>& path, const Color& col, float w) {
        for (std::size_t i = 0; i + 1 < path.size(); ++i) {
            Vec2 p0 = W(path[i]), p1 = W(path[i + 1]);
            if (cohenSutherland(p0, p1, view)) D::line(p0, p1, col, w);
        }
        for (std::size_t i = 1; i + 1 < path.size(); ++i) {
            Vec2 p = W(path[i]);
            if (view.contains(p)) D::circle(p, w * 0.5f, col, 8);
        }
    };
    for (int b = 0; b < BUS_COUNT; ++b) {
        if (activeBus[b]) continue;
        bool status = b == B_FLAGS_CU || b == B_IR_PC;
        drawPath(busPath(b), (status ? theme::violet : theme::wire).alpha(status ? 0.35f : 1.f), std::max(1.5f, 2.6f * s));
    }
    // ---- control lines (dashed); lit when their signal is asserted
    for (const auto& cl : controlLines()) {
        bool on = ms && sim.animating() && (cl.signal < 0 ? ms->sig.aluOp != AluOp::None : ms->sig.get(cl.signal));
        Color col = on ? theme::violet.lighten(0.25f) : theme::violet.alpha(0.22f);
        for (std::size_t i = 0; i + 1 < cl.path.size(); ++i) {
            Vec2 p0 = W(cl.path[i]), p1 = W(cl.path[i + 1]);
            if (cohenSutherland(p0, p1, view))
                D::dashedLine(p0, p1, col, on ? std::max(1.5f, 2.f * s) : std::max(1.f, 1.2f * s), 6 * s, 4 * s,
                              on ? -ui.time * 40 : 0);
        }
        Vec2 end = W(cl.path.back());
        D::arrowHead(end, W(cl.path.back()) - W(cl.path[cl.path.size() - 2]), 7 * s, col);
        if (on) {
            Vec2 lp = W(cl.path[cl.path.size() - 2]);
            std::string label = cl.signal < 0 ? std::string("ALUOp=") + aluOpName(ms->sig.aluOp) : cl.name;
            float tw = D::textWidth(Face::Bold, pxs, label) + 10;
            Rect lr(lp.x + 4, lp.y - pxs - 10, tw, pxs + 8);
            D::roundRect(lr, 5, theme::overlay.alpha(0.95f));
            D::roundRectOutline(lr, 5, theme::violet.alpha(0.7f));
            D::textIn(Face::Bold, pxs, lr, label, theme::violet.lighten(0.4f), Align::Center);
        }
    }

    // ---- units
    auto unitBox = [&](int u, const std::string& title) {
        Rect r = WR(blockRect(u));
        float g = unitGlow_[u];
        if (g > 0.02f) D::glow(r, 10 * s, 14 * s * g, pcol.alpha(g));
        D::roundRectGrad(r, 9 * s, theme::unitTop.lerp(pcol, 0.18f * g), theme::unitBottom.lerp(pcol, 0.08f * g));
        D::roundRectOutline(r, 9 * s, theme::unitBorder.lerp(pcol, g), 1 + g);
        D::text(Face::Bold, px, r.x + 8 * s, r.y + 6 * s, title, theme::text.lerp(pcol.lighten(0.4f), g));
        return r;
    };

    // PC
    {
        Rect r = unitBox(U_PC, "PC");
        D::text(Face::MonoBold, pxs, r.x + 8 * s, r.y + 28 * s,
                hex64(CPU::addressOf(sim.hasCurrent ? sim.current.index : sim.cpu.pc())).substr(14), theme::cyan);
        // +4 adder
        Vec2 ac = W({55, 152});
        float ar = 24 * s;
        float g = (activeBus[B_PC_INC] || activeBus[B_INC_PC]) ? 1.f : 0.f;
        D::circle(ac, ar, theme::unitTop.lerp(pcol, 0.25f * g));
        D::ring(ac, ar, 1.5f + g, theme::unitBorder.lerp(pcol, g));
        D::text(Face::Bold, px, ac.x, ac.y - px * 0.55f, "+4", theme::text, Align::Center);
    }
    // instruction memory
    {
        Rect r = unitBox(U_IMEM, "Instr Memory");
        if (sim.program) {
            int cur = sim.hasCurrent ? sim.current.index : sim.cpu.pc();
            for (int k = -2; k <= 2; ++k) {
                int i = cur + k;
                if (i < 0 || i >= int(sim.program->code.size())) continue;
                float y = r.y + (30 + (k + 2) * 21) * s;
                if (k == 0) D::roundRect(Rect(r.x + 4 * s, y - 2 * s, r.w - 8 * s, 19 * s), 4 * s, theme::blue.alpha(0.25f));
                std::string t = sim.program->code[i]->text();
                D::text(Face::Mono, std::max(7, int(10 * s)), r.x + 8 * s, y,
                        D::ellipsize(Face::Mono, std::max(7, int(10 * s)), t, r.w - 14 * s),
                        k == 0 ? theme::text : theme::textMute);
            }
        }
    }
    // IR
    {
        Rect r = unitBox(U_IR, "IR");
        std::string t = sim.hasCurrent ? sim.current.mnemonic : "--";
        D::text(Face::MonoBold, px, r.x + 8 * s, r.y + 30 * s, t, theme::amber);
    }
    // control unit
    {
        Rect r = unitBox(U_CU, "Control Unit");
        D::text(Face::Semibold, pxs, r.r() - 8 * s, r.y + 7 * s, sim.cu->name(), theme::violet.lighten(0.3f), Align::Right);
        if (ms) {
            std::string mo = ms->microOp;
            D::text(Face::Mono, std::max(7, int(11 * s)), r.x + 8 * s, r.y + 32 * s,
                    D::ellipsize(Face::Mono, std::max(7, int(11 * s)), mo, r.w - 16 * s), theme::textDim);
            if (ms->microAddr >= 0) {
                char b[32];
                std::snprintf(b, sizeof b, "uPC = %d", ms->microAddr);
                D::text(Face::MonoBold, pxs, r.x + 8 * s, r.y + 54 * s, b, theme::violet.lighten(0.3f));
            }
        }
    }
    // register file
    {
        Rect r = unitBox(U_REGS, "Register File");
        std::vector<int> regs;
        if (sim.hasCurrent) {
            for (int x : sim.current.regsRead) if (std::find(regs.begin(), regs.end(), x) == regs.end()) regs.push_back(x);
            for (int x : sim.current.regsWritten) if (std::find(regs.begin(), regs.end(), x) == regs.end()) regs.push_back(x);
        }
        if (regs.empty()) regs = {RAX, RBX, RCX, RDX};
        for (int k = 0; k < std::min(5, int(regs.size())); ++k) {
            int rg = regs[k];
            float y = r.y + (32 + k * 29) * s;
            bool w = sim.hasCurrent && std::find(sim.current.regsWritten.begin(), sim.current.regsWritten.end(), rg) !=
                                           sim.current.regsWritten.end();
            Rect rr(r.x + 6 * s, y, r.w - 12 * s, 24 * s);
            D::roundRect(rr, 5 * s, (w ? theme::amber : theme::cyan).alpha(0.12f + 0.15f * sim.regFlash[rg]));
            D::text(Face::MonoBold, pxs, rr.x + 6 * s, rr.y + 4 * s, regName(rg), w ? theme::amber : theme::ident);
            D::text(Face::Mono, pxs, rr.r() - 6 * s, rr.y + 4 * s, fmtValue(std::int64_t(sim.shownRegs[rg])), theme::text,
                    Align::Right);
        }
    }
    // immediate
    {
        Rect r = unitBox(U_IMM, "Imm / Sign-ext");
        if (sim.hasCurrent && sim.current.immUsed)
            D::text(Face::MonoBold, pxs, r.r() - 8 * s, r.y + 26 * s, fmtValue(sim.current.immValue), theme::amber, Align::Right);
    }
    // MUX + ALU (scanline filled)                                       [CG CO3]
    auto symbol = [&](int u, const std::vector<Vec2>& worldPoly) {
        std::vector<Vec2> poly;
        for (auto p : worldPoly) poly.push_back(W(p));
        Rect r = WR(blockRect(u));
        float g = unitGlow_[u];
        if (g > 0.02f) D::glow(r.inset(6 * s), 10 * s, 14 * s * g, pcol.alpha(g));
        fillScanline(poly, theme::unitTop.lerp(pcol, 0.35f * g), theme::unitBottom.lerp(pcol, 0.15f * g), r.y, r.b());
        std::vector<Vec2> closed = poly;
        closed.push_back(poly.front());
        D::polyline(closed, theme::unitBorder.lerp(pcol, g), 1.6f + g);
        return r;
    };
    {
        Rect r = symbol(U_MUX, muxShape(blockRect(U_MUX)));
        bool sel = ms && sim.animating() && ms->sig.ALUSrc;
        D::text(Face::Bold, pxs, r.center().x, r.y + 10 * s, "M", theme::textDim, Align::Center);
        D::text(Face::MonoBold, pxs, r.center().x, r.center().y - 6 * s, sel ? "1" : "0", sel ? theme::amber : theme::textMute,
                Align::Center);
    }
    {
        Rect r = symbol(U_ALU, aluShape(blockRect(U_ALU)));
        AluOp op = (ms && sim.animating() && ms->sig.aluOp != AluOp::None) ? ms->sig.aluOp
                   : sim.hasCurrent ? sim.current.aluOp : AluOp::None;
        D::text(Face::Bold, px, r.x + r.w * 0.6f, r.y + r.h * 0.33f, "ALU", theme::text, Align::Center);
        std::string sym = aluOpSymbol(op);
        if (!sym.empty())
            D::text(Face::Bold, int(px * 2.2f), r.x + r.w * 0.6f, r.center().y - px * 0.9f, sym,
                    theme::amber.lerp(pcol, unitGlow_[U_ALU] * 0.5f), Align::Center);
        D::text(Face::MonoBold, pxs, r.x + r.w * 0.6f, r.y + r.h * 0.62f, aluOpName(op), theme::textDim, Align::Center);
        if (sim.hasCurrent && sim.current.aluUsed)
            D::text(Face::MonoBold, pxs, r.x + r.w * 0.6f, r.y + r.h * 0.72f, "= " + fmtValue(std::int64_t(sim.current.aluOut)),
                    theme::green, Align::Center);
    }
    // flags
    {
        Rect r = unitBox(U_FLAGS, "Flags");
        const Flags& f = sim.shownFlags;
        const std::pair<const char*, bool> fl[] = {{"C", f.CF}, {"Z", f.ZF}, {"S", f.SF}, {"O", f.OF}};
        const Color fc[] = {theme::amber, theme::green, theme::pink, theme::red};
        for (int k = 0; k < 4; ++k) {
            Vec2 cc(r.x + (18 + k * 25) * s, r.y + 38 * s);
            float rad = 7 * s;
            if (fl[k].second) { D::circle(cc, rad * 1.8f, fc[k].alpha(0.18f)); D::circle(cc, rad, fc[k]); }
            // LED ring by midpoint circle                                [CG CO3]
            std::vector<Vec2> pts;
            for (auto p : midpointCircle(int(cc.x), int(cc.y), int(rad + 1))) pts.push_back({p.x + 0.5f, p.y + 0.5f});
            D::points(pts, 1.5f, fl[k].second ? fc[k].lighten(0.3f) : theme::borderHi);
            D::text(Face::Bold, std::max(7, int(9 * s)), cc.x, cc.y + rad + 2 * s, fl[k].first, theme::textMute, Align::Center);
        }
    }
    // data memory
    {
        Rect r = unitBox(U_DMEM, "Data Memory");
        if (sim.hasCurrent && !sim.current.mem.empty()) {
            int k = 0;
            for (const auto& m : sim.current.mem) {
                if (k >= 3) break;
                float y = r.y + (34 + k * 40) * s;
                Rect mr(r.x + 6 * s, y, r.w - 12 * s, 34 * s);
                D::roundRect(mr, 5 * s, (m.write ? theme::pink : theme::cyan).alpha(0.14f));
                char ad[40];
                std::snprintf(ad, sizeof ad, "%s [0x%llX]", m.write ? "WR" : "RD", static_cast<unsigned long long>(m.addr));
                D::text(Face::MonoBold, std::max(7, int(10 * s)), mr.x + 6 * s, mr.y + 3 * s, ad, m.write ? theme::pink : theme::cyan);
                D::text(Face::Mono, pxs, mr.x + 6 * s, mr.y + 17 * s, fmtValue(m.value), theme::text);
                ++k;
            }
        } else {
            D::text(Face::Regular, pxs, r.x + 8 * s, r.y + 34 * s, "no access", theme::textMute);
        }
    }
    {
        Rect r = symbol(U_WB, muxShape(blockRect(U_WB)));
        bool sel = ms && sim.animating() && ms->sig.MemToReg;
        D::text(Face::Bold, pxs, r.center().x, r.y + 10 * s, "WB", theme::textDim, Align::Center);
        D::text(Face::MonoBold, pxs, r.center().x, r.center().y - 6 * s, sel ? "1" : "0", sel ? theme::pink : theme::textMute,
                Align::Center);
    }

    // ---- active buses: Bresenham signal dots + value packet           [CG CO3]
    if (ms && sim.animating()) {
        float t = easeInOutCubic(sim.stepT);
        int labelBus = ms->buses.empty() ? -1 : ms->buses.back();
        for (int b : ms->buses) {
            std::vector<Vec2> world = busPath(b);
            if (world.empty()) continue;
            std::vector<Vec2> scr;
            for (auto p : world) scr.push_back(W(p));
            // bright core line (clipped)
            for (std::size_t i = 0; i + 1 < scr.size(); ++i) {
                Vec2 p0 = scr[i], p1 = scr[i + 1];
                if (!cohenSutherland(p0, p1, view)) continue;
                D::line(p0, p1, pcol.alpha(0.35f), std::max(4.f, 7 * s));
                D::line(p0, p1, pcol.lighten(0.2f), std::max(2.f, 3 * s));
                // flowing dots on the rasterised pixels
                std::vector<IPoint> pix = bresenhamLine(int(p0.x), int(p0.y), int(p1.x), int(p1.y));
                std::vector<Vec2> dots;
                int spacing = std::max(8, int(14 * s));
                int phase = int(ui.time * 90) % spacing;
                for (int k = phase; k < int(pix.size()); k += spacing) dots.push_back({pix[k].x + 0.5f, pix[k].y + 0.5f});
                D::points(dots, std::max(3.f, 4.5f * s), Color(1, 1, 1, 0.85f));
            }
            // packet travelling along the wire
            float L = pathLength(scr);
            Vec2 p = pointAlong(scr, L * t);
            if (view.contains(p)) {
                D::circle(p, 9 * s, pcol.alpha(0.3f));
                D::circle(p, 5 * s, Color(1, 1, 1, 1));
                if (b == labelBus && !ms->value.empty()) {
                    std::string v = D::ellipsize(Face::MonoBold, pxs, ms->value, 180 * s);
                    float tw = D::textWidth(Face::MonoBold, pxs, v) + 14;
                    Rect lr(p.x - tw / 2, p.y - 30 * s, tw, pxs + 10);
                    D::shadow(lr, 6, 8, Color(0, 0, 0, 1));
                    D::roundRect(lr, 6, theme::overlay.alpha(0.96f));
                    D::roundRectOutline(lr, 6, pcol);
                    D::textIn(Face::MonoBold, pxs, lr, v, pcol.lighten(0.45f), Align::Center);
                }
            }
            // bus name near its middle
            const char* bl = busLabel(b);
            if (*bl) {
                Vec2 mpt = pointAlong(scr, L * 0.5f);
                D::text(Face::Semibold, std::max(7, int(10 * s)), mpt.x + 6, mpt.y + 4, bl, pcol.lighten(0.3f));
            }
        }
    }
    D::popClip();

    if (!sim.loaded())
        ui.emptyState(c, "No program loaded", "Compile a statement on the Compiler tab, then press Step or Run.");
}

void DatapathView::drawInfo(const Rect& area, Sim& sim, UI& ui) {
    Rect a = area;
    float topH = std::min(250.f, std::max(214.f, a.h * 0.42f));
    const float sigH = 134;
    // short screens: give the decoder table / control store at least ~150 px
    if (a.h - topH - sigH - 2 * theme::gap < 150) topH = std::max(168.f, a.h - sigH - 2 * theme::gap - 150);
    Rect top = a.cutTop(topH);
    a.cutTop(theme::gap);
    Rect sig = a.cutTop(sigH);
    a.cutTop(theme::gap);
    Rect detail = a;

    const MicroStep* ms = sim.currentStep();
    Rect c = ui.card(top, "CURRENT INSTRUCTION", theme::amber,
                     sim.hasCurrent ? std::string(instrClassName(sim.current.klass)) : "");
    if (!sim.hasCurrent) {
        ui.emptyState(c, "Nothing executing", "Press Step to fetch the next instruction or Run to play the program.");
    } else {
        drawAsmLine(c.x, c.y - 2, sim.current.text, 1.f, 17);
        D::text(Face::Mono, 11, c.x, c.y + 24, hex64(sim.current.pcAddr) + "   step #" + std::to_string(sim.current.stepNo),
                theme::textMute);
        // phase timeline
        float y = c.y + 48;
        const Phase phases[5] = {Phase::Fetch, Phase::Decode, Phase::Execute, Phase::Memory, Phase::WriteBack};
        const char* short_[5] = {"IF", "ID", "EX", "MEM", "WB"};
        float pw = (c.w - 4 * 6) / 5;
        for (int i = 0; i < 5; ++i) {
            bool used = false, done = true, now = false;
            for (int k = 0; k < int(sim.steps.size()); ++k) {
                if (sim.steps[k].phase != phases[i]) continue;
                used = true;
                if (k >= sim.stepIdx) done = false;
                if (k == sim.stepIdx && sim.animating()) now = true;
            }
            Rect pr(c.x + i * (pw + 6), y, pw, 30);
            Color pc = theme::phaseColor(phases[i]);
            if (!used) {
                D::roundRect(pr, 8, theme::bg1);
                D::textIn(Face::Bold, 11, pr, short_[i], theme::textMute.alpha(0.5f), Align::Center);
            } else if (now) {
                D::glow(pr, 8, 8, pc);
                D::roundRectGrad(pr, 8, pc.lighten(0.15f), pc);
                D::textIn(Face::Bold, 12, pr, short_[i], theme::onAccent, Align::Center);
            } else {
                D::roundRect(pr, 8, pc.alpha(done ? 0.28f : 0.1f));
                D::roundRectOutline(pr, 8, pc.alpha(0.5f));
                D::textIn(Face::Bold, 12, pr, short_[i], done ? pc.lighten(0.4f) : theme::textDim, Align::Center);
            }
        }
        D::text(Face::Semibold, 11, c.x, y + 36,
                std::to_string(std::min(sim.stepIdx + 1, int(sim.steps.size()))) + " / " + std::to_string(sim.steps.size()) +
                    (sim.micro ? " micro-instructions" : " cycles"),
                theme::textMute);
        // progress bar of the current micro-step
        Rect pb(c.x + 150, y + 41, c.w - 150, 4);
        D::roundRect(pb, 2, theme::bg1);
        if (sim.animating()) D::roundRect(Rect(pb.x, pb.y, pb.w * sim.stepT, pb.h), 2, ms ? theme::phaseColor(ms->phase) : theme::cyan);
        if (ms) {
            Rect mo(c.x, y + 56, c.w, 34);
            D::roundRect(mo, 8, theme::bg1);
            D::roundRectOutline(mo, 8, theme::phaseColor(ms->phase).alpha(0.45f));
            D::textIn(Face::MonoBold, 13, mo, D::ellipsize(Face::MonoBold, 13, ms->microOp, mo.w - 20),
                      theme::phaseColor(ms->phase).lighten(0.4f), Align::Left, 10);
            if (y + 98 + 16 < top.b() - 8)
                D::paragraph(Face::Regular, 12, Rect(c.x, y + 98, c.w, 60), ms->detail, theme::textDim);
            else if (ui.hovered(mo))
                ui.tooltip(ms->detail);
        }
    }

    // control signals
    Rect sc = ui.card(sig, "CONTROL SIGNALS", theme::violet, ms ? phaseName(ms->phase) : "");
    float x = sc.x, y = sc.y;
    ControlSignals cs = (ms && sim.animating()) ? ms->sig : ControlSignals{};
    for (int i = 0; i < ControlSignals::kCount; ++i) {
        float w = D::textWidth(Face::MonoBold, 11, ControlSignals::name(i)) + 18;
        if (x + w > sc.r()) { x = sc.x; y += 28; }
        ui.chip(x, y, ControlSignals::name(i), theme::violet, cs.get(i), 11);
        x += w + 6;
    }
    std::string alu = std::string("ALUOp=") + aluOpName(cs.aluOp);
    float w = D::textWidth(Face::MonoBold, 11, alu) + 18;
    if (x + w > sc.r()) { x = sc.x; y += 28; }
    ui.chip(x, y, alu, theme::amber, cs.aluOp != AluOp::None, 11);

    drawControlDetail(detail, sim, ui);
}

void DatapathView::drawControlDetail(const Rect& r, Sim& sim, UI& ui) {
    const MicroStep* ms = sim.currentStep();
    if (!sim.micro) {
        Rect c = ui.card(r, "HARDWIRED DECODER TABLE", theme::violet, "opcode \xE2\x86\x92 signals");
        const auto& rows = HardwiredControlUnit::decoderTable();
        int cur = sim.hasCurrent && sim.program ? HardwiredControlUnit::decoderRow(*sim.program->code[sim.current.index]) : -1;
        const char* hdr[] = {"PW", "IW", "MR", "MW", "RW", "AS", "MR", "BR", "JP", "FW", "SO"};
        float formW = 116;
        float cw = (c.w - formW) / ControlSignals::kCount;
        for (int i = 0; i < ControlSignals::kCount; ++i)
            D::text(Face::Bold, 9, c.x + formW + i * cw + cw / 2, c.y - 4, hdr[i], theme::textMute, Align::Center);
        float rowH = 17;
        // keep the selected row visible (centred when possible)
        int visible = std::max(1, int((c.h - 14) / rowH));
        int first = 0;
        if (cur >= 0) first = std::max(0, std::min(int(rows.size()) - visible, cur - visible / 2));
        D::pushClip(Rect(c.x - 4, c.y + 10, c.w + 8, c.h - 10));
        for (int k = first; k < int(rows.size()); ++k) {
            float y = c.y + 12 + (k - first) * rowH;
            Rect rr(c.x - 4, y, c.w + 8, rowH - 1);
            bool sel = k == cur;
            if (sel) {
                D::roundRect(rr, 4, theme::violet.alpha(0.25f));
                D::roundRectOutline(rr, 4, theme::violet.alpha(0.7f));
            }
            D::text(Face::Mono, 10, c.x, y + 2, rows[k].form, sel ? theme::text : theme::textDim);
            for (int i = 0; i < ControlSignals::kCount; ++i) {
                Vec2 cc(c.x + formW + i * cw + cw / 2, y + rowH / 2);
                if (rows[k].sig.get(i)) D::circle(cc, 3.5f, sel ? theme::violet.lighten(0.4f) : theme::violet.alpha(0.6f));
                else D::circle(cc, 1.5f, theme::textMute.alpha(0.4f));
            }
        }
        D::popClip();
    } else {
        auto* mp = dynamic_cast<MicroprogrammedControlUnit*>(sim.cu.get());
        Rect c = ui.card(r, "CONTROL STORE (MICRO-CODE ROM)", theme::violet, mp ? std::to_string(mp->controlStore().size()) + " words" : "");
        if (!mp) return;
        const auto& store = mp->controlStore();
        int upc = ms ? ms->microAddr : 0;
        float rowH = 20;
        int visible = int(c.h / rowH);
        int first = std::max(0, std::min(int(store.size()) - visible, upc - visible / 2));
        float scroll = ui.anim("ustoreScroll", float(first), 10);
        D::pushClip(Rect(c.x - 4, c.y, c.w + 8, c.h));
        for (int k = 0; k < int(store.size()); ++k) {
            float y = c.y + (k - scroll) * rowH;
            if (y < c.y - rowH || y > c.b()) continue;
            const MicroInstr& mi = store[k];
            bool sel = k == upc && sim.hasCurrent;
            Rect rr(c.x - 4, y, c.w + 8, rowH - 2);
            Color pc = theme::phaseColor(mi.phase);
            if (sel) {
                D::glow(rr, 4, 5, pc);
                D::roundRect(rr, 4, pc.alpha(0.3f));
            } else if (k == 0 || k == 3) {
                D::rect(Rect(rr.x, rr.y - 1, rr.w, 1), theme::border);
            }
            char addr[8];
            std::snprintf(addr, sizeof addr, "%02d", mi.addr);
            D::text(Face::MonoBold, 10, c.x, y + 3, addr, sel ? theme::text : theme::textMute);
            D::text(Face::MonoBold, 10, c.x + 22, y + 3, mi.label, sel ? pc.lighten(0.5f) : pc.alpha(0.8f));
            D::text(Face::Mono, 10, c.x + 72, y + 3, D::ellipsize(Face::Mono, 10, mi.microOp, c.w - 72 - 50),
                    sel ? theme::text : theme::textDim);
            D::text(Face::Mono, 9, c.r(), y + 4, mi.next == "next" ? "" : mi.next, theme::textMute, Align::Right);
        }
        D::popClip();
    }
}

void DatapathView::drawWaveform(const Rect& r, Sim& sim, UI& ui) {
    Rect c = ui.card(r, "CONTROL SIGNAL WAVEFORM", theme::teal,
                     "logic analyser  \xC2\xB7  one column per clock cycle  \xC2\xB7  " + std::to_string(sim.wave.size()) + " samples");
    const int nSig = ControlSignals::kCount;
    float labelW = 78;
    Rect plot(c.x + labelW, c.y, c.w - labelW, c.h);
    int rows = nSig + 2;   // CLK + signals + phase
    float rowH = plot.h / rows;
    float colW = 34;
    int n = int(sim.wave.size());
    // scroll so that the newest sample slides in smoothly
    float progress = sim.animating() ? sim.stepT : 1.f;
    float totalW = (n + progress) * colW;
    float offset = std::max(0.f, totalW - plot.w);
    auto X = [&](float col) { return plot.x + col * colW - offset; };

    // labels + grid
    const char* extra[2] = {"CLK", "PHASE"};
    for (int i = 0; i < rows; ++i) {
        float y = plot.y + i * rowH;
        std::string name = i == 0 ? extra[0] : (i == rows - 1 ? extra[1] : ControlSignals::name(i - 1));
        D::text(Face::MonoBold, 10, c.x, y + rowH / 2 - 6, name, i == 0 ? theme::teal : theme::textMute);
        if (i % 2 == 1) D::rect(Rect(plot.x, y, plot.w, rowH), Color(1, 1, 1, 0.018f));
    }
    for (int k = 0; k <= n; ++k) {
        float x = X(float(k));
        if (x < plot.x || x > plot.r()) continue;
        D::rect(Rect(x, plot.y, 1, plot.h), Color(1, 1, 1, 0.04f));
    }
    // traces: each high/low segment is clipped with Liang-Barsky         [CG CO5]
    auto seg = [&](Vec2 p0, Vec2 p1, const Color& col, float w) {
        if (liangBarsky(p0, p1, plot)) D::line(p0, p1, col, w);
    };
    // clock
    {
        float yH = plot.y + rowH * 0.2f, yL = plot.y + rowH * 0.8f;
        float total = n + progress;
        for (int k = 0; k < int(std::ceil(total)) + 1; ++k) {
            float x0 = X(float(k)), xm = X(k + 0.5f), x1 = X(k + 1.f);
            float lim = X(total);
            seg({x0, yH}, {std::min(xm, lim), yH}, theme::teal, 1.6f);
            if (xm < lim) { seg({xm, yH}, {xm, yL}, theme::teal, 1.6f); seg({xm, yL}, {std::min(x1, lim), yL}, theme::teal, 1.6f); }
            if (x1 < lim) seg({x1, yL}, {x1, yH}, theme::teal, 1.6f);
        }
    }
    for (int sIdx = 0; sIdx < nSig; ++sIdx) {
        float yH = plot.y + (sIdx + 1) * rowH + rowH * 0.22f, yL = plot.y + (sIdx + 2) * rowH - rowH * 0.22f;
        bool prev = false;
        for (int k = 0; k < n; ++k) {
            const SignalSample& smp = sim.wave.at(std::size_t(k));
            bool v = smp.sig.get(sIdx);
            float x0 = X(float(k)), x1 = X(k + 1.f);
            Color col = theme::phaseColor(smp.phase);
            if (v != prev && k > 0) seg({x0, prev ? yH : yL}, {x0, v ? yH : yL}, v ? col : theme::textMute, 1.4f);
            if (v) {
                Rect fill(x0, yH, x1 - x0, yL - yH);
                Rect cl = fill.intersect(plot);
                if (cl.w > 0) D::rect(cl, col.alpha(0.16f));
                seg({x0, yH}, {x1, yH}, col, 2.0f);
            } else {
                seg({x0, yL}, {x1, yL}, theme::textMute.alpha(0.6f), 1.2f);
            }
            prev = v;
        }
    }
    // phase row + instruction boundaries
    float py = plot.y + (rows - 1) * rowH + 2;
    for (int k = 0; k < n; ++k) {
        const SignalSample& smp = sim.wave.at(std::size_t(k));
        float x0 = X(float(k)), x1 = X(k + 1.f);
        Rect pr(x0 + 1, py, x1 - x0 - 2, rowH - 4);
        Rect cl = pr.intersect(plot);
        if (cl.w <= 0) continue;
        D::rect(cl, theme::phaseColor(smp.phase).alpha(0.4f));
        if (smp.boundary) {
            Vec2 a(x0, plot.y), b(x0, plot.b());
            if (liangBarsky(a, b, plot)) D::line(a, b, theme::amber.alpha(0.35f), 1.f);
            if (x0 + 4 > plot.x && x0 < plot.r() - 30)
                D::text(Face::MonoBold, 9, x0 + 3, plot.y - 1, smp.label, theme::amber.alpha(0.8f));
        }
    }
    if (n == 0) D::textIn(Face::Regular, 13, plot, "Signals are recorded here as the program runs", theme::textMute, Align::Center);
}

} // namespace vas::ui
