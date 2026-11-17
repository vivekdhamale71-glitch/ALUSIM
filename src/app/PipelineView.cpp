// =====================================================================
//  Pipeline view                                          [COA CO4]
//  5-stage pipeline computed from the real execution trace:
//  stage boxes with pipeline registers for the current cycle, the
//  space-time diagram with stalls, flushes and forwarding paths, and
//  the speed-up compared with the non-pipelined processor.
// =====================================================================
#include <algorithm>
#include <cmath>
#include <cstdio>

#include "CG_2310218L/CO4_ProjectionViewing/Scene3D.h"
#include "app/Views.h"

namespace vas::ui {

using namespace gfx;
using namespace core;
namespace D = gfx::draw;

static void statTile(const Rect& r, const std::string& label, const std::string& value, const std::string& sub,
                     const Color& c) {
    D::shadow(r, 12, 12, Color(0, 0, 0, 1));
    D::roundRectGrad(r, 12, theme::panel.lighten(0.04f), theme::panel);
    D::roundRectOutline(r, 12, theme::border);
    D::rect(Rect(r.x + 14, r.y + 12, 3, r.h - 24), c);
    D::text(Face::Bold, 11, r.x + 26, r.y + 12, label, theme::textMute);
    D::text(Face::Bold, 26, r.x + 26, r.y + 28, value, theme::text);
    D::text(Face::Regular, 11, r.x + 26, r.b() - 22, sub, c.lighten(0.25f));
}

void PipelineView::draw(const Rect& area, Sim& sim, UI& ui) {
    Rect a = area;
    if (!sim.loaded()) {
        Rect c = ui.card(a, "PIPELINE", theme::amber);
        ui.emptyState(c, "No program loaded", "Compile a program first. The pipeline is built from its real execution trace.");
        return;
    }
    if (sim.trace.empty() || !sim.cpu.halted()) {
        // the diagram needs the dynamic trace; offer to run the program
        if (sim.trace.empty()) {
            Rect c = ui.card(a, "PIPELINE", theme::amber);
            ui.emptyState(Rect(c.x, c.y - 60, c.w, c.h), "Run the program first",
                          "The 5-stage pipeline is computed from the instructions the CPU actually executed "
                          "(loops repeat, branches jump), so the program has to run once.");
            Rect b(c.center().x - 110, c.center().y + 70, 220, 42);
            if (ui.button("pipeRun", b, "Run whole program", Style::Primary, int(Icon::Finish))) sim.finish();
            return;
        }
    }
    const PipelineResult& P = sim.pipeline(forwarding_, singlePort_);
    if (seenGen_ != sim.runGen) { seenGen_ = sim.runGen; cycle_ = 0; playing_ = true; }
    int maxCycle = 1;
    for (const auto& row : P.rows)
        for (int s = 0; s < STAGE_COUNT; ++s) maxCycle = std::max(maxCycle, row.end[s]);
    if (playing_) {
        cycle_ += ui.dt * 2.2f * std::max(0.5f, sim.speed);
        if (cycle_ >= maxCycle) { cycle_ = float(maxCycle); playing_ = false; }
    }
    int cyc = std::max(1, int(cycle_) + 1);

    // ------------------------------------------------------------- top row
    Rect top = a.cutTop(118);
    a.cutTop(theme::gap);
    Rect settings = top.cutLeft(330);
    top.cutLeft(theme::gap);
    Rect sc = ui.card(settings, "HAZARD HANDLING", theme::amber);
    bool f = forwarding_, sp = singlePort_;
    ui.toggle("fwd", Rect(sc.x, sc.y - 4, sc.w, 28), forwarding_, "Data forwarding (bypass)");
    ui.toggle("port", Rect(sc.x, sc.y + 26, sc.w, 28), singlePort_, "Single memory port (structural)");
    if (f != forwarding_ || sp != singlePort_) { cycle_ = 0; playing_ = true; }
    if (!sim.cpu.halted())
        D::text(Face::Regular, 11, sc.x, sc.y + 58, "trace so far: " + std::to_string(sim.trace.size()) + " instructions",
                theme::amber);

    float tw = (top.w - 4 * 10) / 5;
    char b1[32], b2[32];
    std::snprintf(b1, sizeof b1, "%.2f", P.cpi());
    std::snprintf(b2, sizeof b2, "%.2fx", P.speedup());
    statTile(Rect(top.x, top.y, tw, top.h), "INSTRUCTIONS", std::to_string(P.instructions), "committed", theme::cyan);
    statTile(Rect(top.x + (tw + 10), top.y, tw, top.h), "NON-PIPELINED", std::to_string(P.nonPipelinedCycles) + " cyc",
             "5 cycles each", theme::textDim);
    statTile(Rect(top.x + 2 * (tw + 10), top.y, tw, top.h), "PIPELINED", std::to_string(P.pipelinedCycles) + " cyc",
             std::to_string(P.stalls) + " stall cycles", theme::amber);
    statTile(Rect(top.x + 3 * (tw + 10), top.y, tw, top.h), "CPI", b1, "ideal = 1.00", theme::violet);
    statTile(Rect(top.x + 4 * (tw + 10), top.y, tw, top.h), "SPEED-UP", b2,
             std::to_string(P.flushes) + " flushed  \xC2\xB7  " + std::to_string(P.forwardsUsed) + " forwards", theme::green);

    // ------------------------------------------------------- stage boxes
    Rect stages = a.cutTop(std::min(250.f, a.h * 0.42f));
    a.cutTop(theme::gap);
    Rect stc = ui.card(stages, "PIPELINE AT CYCLE " + std::to_string(cyc), theme::blue,
                       "IF \xE2\x86\x92 ID \xE2\x86\x92 EX \xE2\x86\x92 MEM \xE2\x86\x92 WB");
    Rect ctl = stc.cutBottom(30);
    // ---- 3D conveyor: five stage platforms, pipeline registers between them,
    //      instruction blocks slide from stage to stage cycle by cycle   [CG CO2, CO3, CO6 | COA CO4]
    {
        const char* stageTitle[5] = {"FETCH", "DECODE", "EXECUTE", "MEMORY", "WRITE-BACK"};
        const char* regName_[4] = {"IF/ID", "ID/EX", "EX/MEM", "MEM/WB"};
        const float SX = 3.3f;
        auto stageX = [&](float s) { return (s - 2) * SX; };
        Rect view(stc.x - 8, stc.y - 6, stc.w + 16, stc.h);
        s3d::Camera cam;
        cam.yaw = -14 + std::sin(ui.time * 0.2f) * 4;
        cam.pitch = 30;
        cam.fov = 30;
        float aspect = view.w / std::max(1.f, view.h);
        cam.dist = std::max(9.f, (5 * SX + 2.5f) / (0.536f * aspect) * 1.18f);
        cam.ty = 0.3f;
        s3d::begin(view, cam, theme::light);
        // platforms + register gates
        for (int s = 0; s < STAGE_COUNT; ++s) {
            Color col = theme::stageColor(s);
            s3d::material(theme::panel3.lerp(col, 0.35f), 0.05f, 25);
            s3d::box(stageX(float(s)) - 1.25f, -0.3f, -1.0f, stageX(float(s)) + 1.25f, 0, 1.0f);
            s3d::material(col, 0.25f);
            s3d::box(stageX(float(s)) - 1.25f, -0.3f, 0.96f, stageX(float(s)) + 1.25f, -0.02f, 1.04f);   // coloured lip
            if (s < 4) {
                float gx = stageX(s + 0.5f);
                s3d::material(theme::borderHi, 0.02f, 40);
                s3d::box(gx - 0.08f, -0.3f, -1.0f, gx + 0.08f, 1.5f, 1.0f);
            }
        }
        // instruction blocks
        float cf = cycle_ + 1;
        int k0 = int(std::floor(cf));
        float frac = easeInOutCubic(cf - k0);
        auto stageAt = [](const PipeRow& row, int c) {
            for (int s = 0; s < STAGE_COUNT; ++s)
                if (row.start[s] >= 0 && row.start[s] <= c && c <= row.end[s]) return s;
            return -1;
        };
        struct Tag { Vec2 p; std::string text; Color col; const char* badge; float alpha; };
        std::vector<Tag> tags;
        for (const auto& row : P.rows) {
            int sa = stageAt(row, k0), sb = stageAt(row, k0 + 1);
            if (row.flushed) {
                if (k0 > row.flushCycle) continue;
                if (k0 == row.flushCycle) sb = -2;   // vanishes
            }
            if (sa < 0 && sb < 0) continue;
            float xa = sa >= 0 ? stageX(float(sa)) : stageX(-1.f);
            float xb = sb >= 0 ? stageX(float(sb)) : (sb == -2 ? xa : stageX(5.f));
            float x = xa + (xb - xa) * frac;
            float alpha = 1;
            if (sa < 0) alpha = frac;
            if (sb < 0) alpha = 1 - frac;
            if (alpha < 0.05f) continue;
            bool stalled = sa >= 0 && sa == sb;
            int s = frac < 0.5f || sb < 0 ? sa : sb;
            Color col = row.flushed ? theme::red : (s >= 0 ? theme::stageColor(s) : theme::cyan);
            if (stalled && !row.flushed) col = theme::orange;
            float pulse = stalled ? 0.5f + 0.5f * std::sin(ui.time * 8) : 0.f;
            float sz = 1.25f * (0.6f + 0.4f * alpha);
            s3d::material(col, 0.18f + 0.3f * pulse);
            s3d::box(x - sz / 2, 0, -sz / 2, x + sz / 2, 0.9f * (0.6f + 0.4f * alpha), sz / 2);
            Tag t;
            s3d::project(x, 1.15f, 0, t.p);
            t.text = row.text;
            t.col = col;
            t.badge = row.flushed ? "FLUSHED" : (stalled ? "STALL" : nullptr);
            t.alpha = alpha;
            tags.push_back(t);
        }
        // stage captions at the front edge of each platform
        std::vector<std::pair<Vec2, int>> caps;
        for (int s = 0; s < STAGE_COUNT; ++s) {
            Vec2 p;
            s3d::project(stageX(float(s)), -0.3f, 1.25f, p);
            caps.push_back({p, s});
        }
        std::vector<std::pair<Vec2, int>> gates;
        for (int s = 0; s < 4; ++s) {
            Vec2 p;
            s3d::project(stageX(s + 0.5f), 1.6f, 0, p);
            gates.push_back({p, s});
        }
        s3d::end();
        for (const auto& c2 : caps) {
            D::text(Face::Bold, 12, c2.first.x, c2.first.y + 2, stageName(c2.second), theme::stageColor(c2.second),
                    Align::Center);
            D::text(Face::Semibold, 9, c2.first.x, c2.first.y + 17, stageTitle[c2.second], theme::textMute, Align::Center);
        }
        for (const auto& g : gates)
            D::text(Face::Bold, 9, g.first.x, g.first.y - 12, regName_[g.second], theme::textMute, Align::Center);
        for (const auto& t : tags) {
            std::string txt = D::ellipsize(Face::MonoBold, 11, t.text, 150);
            float w = D::textWidth(Face::MonoBold, 11, txt) + 14;
            Rect r(t.p.x - w / 2, t.p.y - 24, w, 20);
            D::roundRect(r, 6, theme::overlay.alpha(0.92f * t.alpha));
            D::roundRectOutline(r, 6, t.col.alpha(0.8f * t.alpha));
            drawAsmLine(r.x + 7, r.y + 3, txt, t.alpha, 11);
            if (t.badge) {
                float bw = D::textWidth(Face::Bold, 9, t.badge) + 12;
                ui.pill(Rect(t.p.x - bw / 2, r.y - 18, bw, 15), t.badge, t.col, true, 9);
            }
        }
    }
    // scrubber
    if (ui.iconButton("pplay", Rect(ctl.x, ctl.y + 2, 30, 28), playing_ ? Icon::Pause : Icon::Play, "Play / pause")) {
        if (cycle_ >= maxCycle) cycle_ = 0;
        playing_ = !playing_;
    }
    if (ui.iconButton("pback", Rect(ctl.x + 34, ctl.y + 2, 30, 28), Icon::Back, "Previous cycle")) {
        playing_ = false;
        cycle_ = std::max(0.f, std::floor(cycle_) - 1);
    }
    if (ui.iconButton("pfwd", Rect(ctl.x + 68, ctl.y + 2, 30, 28), Icon::Step, "Next cycle")) {
        playing_ = false;
        cycle_ = std::min(float(maxCycle), std::floor(cycle_) + 1);
    }
    float cv = cycle_;
    Rect sl(ctl.x + 112, ctl.y + 4, ctl.w - 112 - 110, 24);
    if (ui.slider("pcyc", sl, cv, 0, float(maxCycle))) { cycle_ = cv; playing_ = false; }
    D::text(Face::MonoBold, 12, ctl.r(), ctl.y + 9, "cycle " + std::to_string(cyc) + " / " + std::to_string(maxCycle),
            theme::textDim, Align::Right);

    // -------------------------------------------------- space-time diagram
    Rect dc = ui.card(a, "SPACE-TIME DIAGRAM", theme::violet,
                      std::to_string(P.rows.size()) + " rows shown" + (P.instructions > 36 ? " (first 36)" : ""));
    float labelW = std::min(250.f, dc.w * 0.26f);
    Rect grid(dc.x + labelW, dc.y + 20, dc.w - labelW, dc.h - 20);
    float cellW = 40;
    float rowH = std::min(26.f, std::max(16.f, grid.h / std::max<std::size_t>(1, P.rows.size())));
    float contentW = maxCycle * cellW;
    float targetScroll = clampf(cycle_ * cellW - grid.w * 0.6f, 0, std::max(0.f, contentW - grid.w));
    scroll_ = approach(scroll_, targetScroll, 6, ui.dt);
    int visibleRows = int(grid.h / rowH);
    // vertical scroll to keep the current rows visible
    int firstRow = 0;
    for (int i = 0; i < int(P.rows.size()); ++i)
        if (P.rows[i].start[S_IF] <= cyc && P.rows[i].end[S_WB] >= cyc) { firstRow = std::max(0, i - visibleRows / 3); break; }
    firstRow = std::min(firstRow, std::max(0, int(P.rows.size()) - visibleRows));
    float vScroll = ui.anim("pipeVS", float(firstRow), 6);
    auto cx = [&](int c) { return grid.x + (c - 1) * cellW - scroll_; };
    auto ry = [&](int r) { return grid.y + (r - vScroll) * rowH; };

    // cycle header
    D::pushClip(Rect(grid.x, dc.y, grid.w, 20));
    for (int c = 1; c <= maxCycle; ++c) {
        float x = cx(c);
        if (x + cellW < grid.x || x > grid.r()) continue;
        D::text(c == cyc ? Face::Bold : Face::Mono, 10, x + cellW / 2, dc.y + 3, std::to_string(c),
                c == cyc ? theme::text : theme::textMute, Align::Center);
    }
    D::popClip();

    // current cycle column
    D::pushClip(grid);
    D::rect(Rect(cx(cyc), grid.y, cellW, grid.h), theme::cyan.alpha(0.08f));
    D::rect(Rect(cx(cyc), grid.y, 1.5f, grid.h), theme::cyan.alpha(0.5f));
    for (int r = 0; r < int(P.rows.size()); ++r) {
        const PipeRow& row = P.rows[r];
        float y = ry(r);
        if (y + rowH < grid.y || y > grid.b()) continue;
        if (r % 2) D::rect(Rect(grid.x, y, grid.w, rowH), Color(1, 1, 1, 0.015f));
        for (int s = 0; s < STAGE_COUNT; ++s) {
            if (row.start[s] < 0) continue;
            for (int c = row.start[s]; c <= row.end[s]; ++c) {
                if (c > cyc) break;                         // not happened yet
                if (row.flushed && c > row.flushCycle) break;
                float x = cx(c);
                if (x + cellW < grid.x || x > grid.r()) continue;
                Rect cell(x + 2, y + 2, cellW - 4, rowH - 4);
                Color col = theme::stageColor(s);
                bool stall = c > row.start[s];
                if (row.flushed) {
                    D::roundRect(cell, 5, theme::red.alpha(0.12f));
                    D::roundRectOutline(cell, 5, theme::red.alpha(0.4f));
                    D::textIn(Face::Bold, 10, cell, stageName(s), theme::red.alpha(0.6f), Align::Center);
                } else if (stall) {
                    D::roundRect(cell, 5, theme::orange.alpha(0.1f));
                    // hatching
                    D::pushClip(cell);
                    for (float k = -cell.h; k < cell.w; k += 6)
                        D::line(Vec2(cell.x + k, cell.b()), Vec2(cell.x + k + cell.h, cell.y), theme::orange.alpha(0.35f), 1.2f);
                    D::popClip();
                    D::roundRectOutline(cell, 5, theme::orange.alpha(0.6f));
                } else {
                    float fresh = c == cyc ? 1.f : 0.f;
                    if (fresh > 0) D::glow(cell, 5, 5, col.alpha(0.6f));
                    D::roundRectGrad(cell, 5, col.alpha(0.55f + 0.25f * fresh), col.alpha(0.35f + 0.2f * fresh));
                    D::textIn(Face::Bold, 10, cell, stageName(s), theme::onAccent, Align::Center);
                }
            }
        }
        if (row.flushed && cyc >= row.flushCycle && row.flushCycle >= 0) {
            float x = cx(row.flushCycle) + cellW;
            D::line(Vec2(cx(row.start[S_IF]) + 4, y + rowH / 2), Vec2(x - 4, y + rowH / 2), theme::red.alpha(0.7f), 1.5f);
        }
    }
    // forwarding arrows (producer EX/MEM -> consumer EX)
    for (const auto& fw : P.forwards) {
        if (fw.cycle > cyc) continue;
        const PipeRow& pr = P.rows[fw.from];
        int pc = pr.end[fw.fromStage];
        Vec2 p0(cx(pc) + cellW - 6, ry(fw.from) + rowH * 0.75f);
        Vec2 p1(cx(fw.cycle) + 8, ry(fw.to) + rowH * 0.3f);
        Color col = fw.fromStage == S_MEM ? theme::pink : theme::green;
        D::bezier(p0, p0 + Vec2(10, rowH * 0.6f), p1 - Vec2(10, rowH * 0.6f), p1, col, 2.0f, 20);
        D::arrowHead(p1, p1 - (p1 - Vec2(10, rowH * 0.6f)), 6, col);
        D::circle(p0, 3, col);
        D::text(Face::MonoBold, 9, (p0.x + p1.x) / 2 + 4, (p0.y + p1.y) / 2 - 6, fw.reg, col.lighten(0.3f));
    }
    D::popClip();

    // row labels
    D::pushClip(Rect(dc.x - 4, grid.y, labelW, grid.h));
    for (int r = 0; r < int(P.rows.size()); ++r) {
        const PipeRow& row = P.rows[r];
        float y = ry(r);
        if (y + rowH < grid.y || y > grid.b()) continue;
        Rect lr(dc.x, y, labelW - 10, rowH);
        bool hov = ui.hovered(lr);
        if (!row.hazard.empty()) {
            D::circle(Vec2(lr.r() - 6, y + rowH / 2), 3.5f, row.flushed ? theme::red : theme::orange);
            if (hov) ui.tooltip(row.hazard);
        }
        drawAsmLine(lr.x, y + (rowH - 15) / 2, D::ellipsize(Face::Mono, 12, row.text, labelW - 28), row.flushed ? 0.4f : 0.95f, 12);
    }
    D::popClip();

    // legend (in the card header, right of the title)
    float lx = dc.x + 190;
    float ly = dc.y - 28;
    const std::pair<const char*, Color> leg[] = {{"stall", theme::orange}, {"flushed", theme::red},
                                                 {"forward EX \xE2\x86\x92 EX", theme::green}, {"forward MEM \xE2\x86\x92 EX", theme::pink}};
    for (const auto& l : leg) {
        D::roundRect(Rect(lx, ly + 4, 10, 10), 3, l.second.alpha(0.7f));
        lx += D::text(Face::Regular, 11, lx + 15, ly + 2, l.first, theme::textMute) + 28;
    }
}

} // namespace vas::ui
