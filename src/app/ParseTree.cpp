// =====================================================================
//  Parse-tree view (2D) + view switching for the chip     [CG CO2, CO4]
//  Replays how the compiler turns each statement into code:
//   1. BUILD   - the postfix items are read one by one (strip at the
//                top). Operands are pushed onto the stack column; every
//                operator pops its operands, links them as children and
//                is pushed back. Nodes glide from the stack into the tree
//                along curved paths (translation animation).
//   2. CODEGEN - the tree is walked in post-order (= postfix order); a
//                value dot travels up each edge to its parent, each node
//                shows where its value lives (rbx / [a] / 5) and its
//                x86-64 lines light up in the code panel.
//  The 3D chip keeps its animated 3D <-> 2D camera switch.
// =====================================================================
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <functional>

#include "CG_2310218L/CO1_Primitives/GL.h"
#include "CG_2310218L/CO4_ProjectionViewing/Camera.h"
#include "app/Views.h"

namespace vas::ui {

using namespace gfx;
using namespace core;
namespace D = gfx::draw;

namespace {

Color nodeColor(int kind) {
    switch (kind) {
        case 0: return theme::pink;
        case 1: return Color::hex(0x38BDF8);
        case 2: return theme::amber;
        case 3: return theme::violet;
        default: return theme::orange;
    }
}

int relopOf(const std::string& ctx, std::string& out) {
    for (const char* r : {">=", "<=", "==", "!=", ">", "<"}) {
        std::size_t p = ctx.find(r);
        if (p != std::string::npos) { out = r; return int(p); }
    }
    out = "?";
    return -1;
}

void partialBezier(Vec2 p0, Vec2 p1, Vec2 p2, Vec2 p3, float upto, const Color& c, float w) {
    std::vector<Vec2> pts;
    int seg = 28;
    for (int i = 0; i <= seg; ++i) {
        float t = upto * i / seg;
        pts.push_back(D::bezierPoint(p0, p1, p2, p3, t));
    }
    D::polyline(pts, c, w);
}

} // namespace

// ----------------------------------------------------------------- camera
void ChipView::applyCamera(const Rect& vp, float yaw, float pitch, float dist, float flatYaw, float flatPitch, float tx,
                           float ty, float tz) {
    float f = easeInOutCubic(flat_);
    float dYaw = std::fmod(flatYaw - yaw + 540.f, 360.f) - 180.f;   // shortest way round
    float y = yaw + dYaw * f, p = pitch + (flatPitch - pitch) * f;
    orbitCamera(vp.w / std::max(1.f, vp.h), y, p, dist, f, tx, ty, tz);   // CG CO4 Camera.cpp
}

// ------------------------------------------------------------------- draw
void ChipView::draw(const Rect& area, Sim& sim, UI& ui) {
    Rect c = ui.card(area, mode_ == 0 ? "PARSE TREE  \xC2\xB7  HOW EACH STATEMENT BECOMES CODE" : "3D PROCESSOR DIE",
                     theme::cyan, "");
    Rect vp = c.inset(-6, -6);
    float target = (mode_ == 1 && want2D_) ? 1.f : 0.f;
    flat_ += (target > flat_ ? 1.f : -1.f) * ui.dt * 1.1f;
    flat_ = clampf(flat_, 0, 1);
    if (std::fabs(flat_ - target) < 0.002f) flat_ = target;

    if (mode_ == 0) drawParse(vp, sim, ui);
    else drawChip(vp, sim, ui);

    Rect ov(vp.x + 16, vp.y + 14, 290, 34);
    int m = ui.segmented("v3mode", ov, {"Parse tree", "3D Chip"}, mode_, theme::cyan);
    if (m != mode_) mode_ = m;
    if (mode_ == 1) {
        Rect dv(ov.r() + 12, ov.y, 150, 34);
        int d = ui.segmented("v3dim", dv, {"3D", "2D"}, want2D_ ? 1 : 0, theme::violet);
        want2D_ = d == 1;
        if (ui.hovered(dv)) ui.tooltip("Animated switch: perspective 3D \xE2\x86\x94 flat orthographic top view (CG CO4)");
    }
}

// ------------------------------------------------------------- scene data
void ChipView::buildScenes(Sim& sim) {
    scenes_.clear();
    const CompileResult& R = sim.compiled;
    if (!R.ok) return;
    struct Acc {
        int stmt;
        std::vector<int> exprs;
        std::vector<int> lines;
    };
    std::vector<Acc> acc;
    for (const auto& e : R.timeline) {
        if (e.kind == TimelineEvent::StmtBegin) acc.push_back({e.a, {}, {}});
        if (acc.empty()) continue;
        if (e.kind == TimelineEvent::Shunt &&
            std::find(acc.back().exprs.begin(), acc.back().exprs.end(), e.a) == acc.back().exprs.end())
            acc.back().exprs.push_back(e.a);
        if (e.kind == TimelineEvent::Gen || e.kind == TimelineEvent::Emit)
            for (int l : e.lines) acc.back().lines.push_back(l);
    }
    for (const auto& a : acc) {
        const StmtTrace& st = R.stmts[a.stmt];
        bool isAssign = st.kind == "assign";
        if (a.exprs.empty() || (!isAssign && a.exprs.size() < 2)) continue;
        ParseScene S;
        S.title = st.source;
        S.lines = a.lines;
        std::vector<int> stack;
        auto push = [&](TreeNode n) {
            n.born = int(S.nodes.size());
            n.slot = int(stack.size());
            S.nodes.push_back(n);
            stack.push_back(int(S.nodes.size()) - 1);
            return int(S.nodes.size()) - 1;
        };
        auto pop = [&](int consumer) {
            int i = stack.back();
            stack.pop_back();
            S.nodes[i].consumed = consumer;
            return i;
        };
        if (isAssign) {
            TreeNode t;
            std::size_t eq = st.source.find_first_of("=+-*/%&|^<>");
            t.label = st.source.substr(0, eq == std::string::npos ? st.source.size() : eq);
            while (!t.label.empty() && t.label.back() == ' ') t.label.pop_back();
            // C++ declarations: "int x = 5" -> x
            std::size_t sp = t.label.rfind(' ');
            if (sp != std::string::npos) t.label = t.label.substr(sp + 1);
            t.kind = 1;
            t.tag = "[" + t.label + "]";
            t.note = "The destination variable " + t.label + " is pushed first and waits at the bottom of the stack";
            push(t);
        }
        for (int ei : a.exprs) {
            const ExprTrace& ex = R.exprs[ei];
            for (int pi = 0; pi < int(ex.postfix.size()); ++pi) {
                const std::string& item = ex.postfix[pi];
                TreeNode n;
                n.label = item;
                bool unary = item == "u-" || item == "~";
                bool isOp = unary || item == "+" || item == "-" || item == "*" || item == "/" || item == "%" ||
                            item == "&" || item == "|" || item == "^" || item == "<<" || item == ">>";
                n.kind = isOp ? 0 : (std::isdigit(static_cast<unsigned char>(item[0])) ? 2 : 1);
                for (const auto& g : ex.gen) {
                    if (g.postfixIndex != pi) continue;
                    n.lines = g.lines;
                    n.tag = g.valueStack.empty() ? "" : g.valueStack.back();
                    n.note = g.note;
                }
                if (isOp) {
                    int self = int(S.nodes.size());
                    int r = unary ? -1 : pop(self);
                    int l = pop(self);
                    n.left = l;
                    n.right = r;
                }
                push(n);
            }
        }
        TreeNode root;
        int self = int(S.nodes.size());
        if (isAssign) {
            root.label = "=";
            root.kind = 3;
            root.note = "'=' joins the destination and the value: the result is stored into memory";
        } else {
            std::string rel;
            relopOf(R.exprs[a.exprs[0]].context, rel);
            root.label = rel;
            root.kind = 4;
            root.note = "CMP subtracts the two sides to set the flags, then a conditional jump skips the block if false";
        }
        int r = pop(self);
        int l = pop(self);
        root.left = l;
        root.right = r;
        root.tag = isAssign ? "stored" : "flags";
        std::vector<int> owned;
        for (const auto& n : S.nodes) owned.insert(owned.end(), n.lines.begin(), n.lines.end());
        for (int ln : S.lines)
            if (std::find(owned.begin(), owned.end(), ln) == owned.end()) root.lines.push_back(ln);
        int ri = push(root);
        S.root = ri;
        stack.pop_back();

        // tidy layout in "leaf" units: leaves left to right, parents centred
        float nextX = 0;
        std::function<void(int, int)> layoutFn = [&](int i, int depth) {
            TreeNode& n = S.nodes[i];
            if (n.left < 0 && n.right < 0) { n.tx = nextX; nextX += 1; }
            else {
                if (n.left >= 0) layoutFn(n.left, depth + 1);
                if (n.right >= 0) layoutFn(n.right, depth + 1);
                float a2 = n.left >= 0 ? S.nodes[n.left].tx : S.nodes[n.right].tx;
                float b2 = n.right >= 0 ? S.nodes[n.right].tx : a2;
                n.tx = (a2 + b2) / 2;
            }
            n.ty = float(depth);
        };
        layoutFn(ri, 0);
        S.width = std::max(1.f, nextX);
        scenes_.push_back(S);
    }
}

// -------------------------------------------------------------- parse view
void ChipView::drawParse(const Rect& vp, Sim& sim, UI& ui) {
    if (seenGen_ != sim.compileGen) {
        seenGen_ = sim.compileGen;
        buildScenes(sim);
        scene_ = 0;
        sceneT_ = 0;
        hold_ = 0;
        scenePlaying_ = true;
    }
    D::roundRect(vp, 12, theme::surface);
    // subtle grid
    {
        std::vector<Vec2> dots;
        for (float y = vp.y + 20; y < vp.b(); y += 24)
            for (float x = vp.x + 20; x < vp.r(); x += 24) dots.push_back({x, y});
        D::points(dots, 1.5f, Color(1, 1, 1, 0.04f));
    }
    if (scenes_.empty()) {
        ui.emptyState(vp, sim.asmMode ? "Assembly mode" : "Nothing compiled yet",
                      sim.asmMode ? "Parse trees are built from English or C++ input. Switch the Compiler tab to "
                                    "English or C++."
                                  : "Compile something on the Compiler tab to see its parse tree being built.");
        return;
    }
    scene_ = std::max(0, std::min(int(scenes_.size()) - 1, scene_));
    const ParseScene& S = scenes_[scene_];
    int nb = int(S.nodes.size());
    float total = float(2 * nb);
    const float stepDur = 0.8f;
    if (scenePlaying_) {
        if (sceneT_ < total) sceneT_ = std::min(total, sceneT_ + ui.dt * std::max(0.5f, sim.speed) / stepDur);
        else if ((hold_ += ui.dt) > 2.5f) {
            scene_ = (scene_ + 1) % int(scenes_.size());
            sceneT_ = 0;
            hold_ = 0;
        }
    }
    float T = sceneT_;
    float evalT = T - nb;
    int evalIdx = evalT >= 0 ? std::min(nb - 1, int(evalT)) : -1;
    bool building = evalT < 0;
    int buildIdx = std::min(nb - 1, int(T));

    // ---------------------------------------------------------- layout
    const CompileResult& R = sim.compiled;
    Rect codeR(vp.r() - 330, vp.y + 112, 314, vp.h - 112 - 90);
    Rect stackR(vp.x + 18, vp.y + 112, 130, vp.h - 112 - 90);
    Rect treeR(stackR.r() + 30, vp.y + 130, codeR.x - stackR.r() - 60, vp.h - 130 - 110);
    int maxDepth = 0, maxSlot = 0;
    for (const auto& n : S.nodes) { maxDepth = std::max(maxDepth, int(n.ty)); maxSlot = std::max(maxSlot, n.slot); }
    float leafGap = clampf(treeR.w / std::max(1.f, S.width), 54, 150);
    float levelGap = clampf(treeR.h / std::max(1, maxDepth), 64, 120);
    if (maxDepth == 0) levelGap = 0;
    float treeW = (S.width - 1) * leafGap;
    float ox = treeR.x + (treeR.w - treeW) / 2;
    float oy = treeR.y + std::max(0.f, (treeR.h - maxDepth * levelGap) / 2);
    float slotH = std::min(54.f, (stackR.h - 40) / std::max(1, maxSlot + 2));
    auto treePos = [&](int i) { return Vec2(ox + S.nodes[i].tx * leafGap, oy + S.nodes[i].ty * levelGap); };
    auto stackPos = [&](int i) { return Vec2(stackR.center().x, stackR.b() - 20 - (S.nodes[i].slot + 0.5f) * slotH); };
    auto moveT = [&](int i) {
        const TreeNode& n = S.nodes[i];
        float start = n.consumed >= 0 ? float(n.consumed) : float(n.born) + 0.35f;
        return easeInOutCubic((T - start) / 0.75f);
    };
    // nodes travel along a curve (quadratic arc) from the stack into the tree
    auto posOf = [&](int i) {
        Vec2 a = stackPos(i), b = treePos(i);
        float k = moveT(i);
        Vec2 ctrl((a.x + b.x) / 2, std::min(a.y, b.y) - 60);
        float u = 1 - k;
        return a * (u * u) + ctrl * (2 * u * k) + b * (k * k);
    };

    // ---------------------------------------------------------- postfix strip
    {
        Rect strip(vp.x + 18, vp.y + 62, codeR.x - vp.x - 36, 38);
        D::roundRect(strip, 10, theme::bg1.alpha(0.9f));
        D::roundRectOutline(strip, 10, theme::border);
        D::text(Face::Bold, 10, strip.x + 12, strip.y + 13, "READING", theme::textMute);
        float x = strip.x + 76;
        for (int i = 0; i < nb; ++i) {
            const TreeNode& n = S.nodes[i];
            std::string lab = n.label;
            float w = D::textWidth(Face::MonoBold, 13, lab) + 18;
            if (x + w > strip.r() - 8) break;
            Rect pr(x, strip.y + 7, w, 24);
            bool cur = building && i == buildIdx;
            bool past = building ? i < buildIdx : true;
            Color col = nodeColor(n.kind);
            if (cur) D::glow(pr, 8, 7, col);
            D::roundRect(pr, 8, col.alpha(cur ? 0.35f : (past ? 0.12f : 0.05f)));
            D::roundRectOutline(pr, 8, col.alpha(cur ? 1.f : 0.35f));
            D::textIn(Face::MonoBold, 13, pr, lab, col.lighten(0.3f).alpha(cur || past ? 1.f : 0.45f), Align::Center);
            x += w + 6;
        }
    }

    // ---------------------------------------------------------- stack column
    {
        D::roundRectGrad(stackR, 14, theme::violet.alpha(0.08f), theme::violet.alpha(0.03f));
        D::roundRectOutline(stackR, 14, theme::violet.alpha(0.4f));
        D::text(Face::Bold, 11, stackR.center().x, stackR.y + 10, "STACK", theme::violet.lighten(0.3f), Align::Center);
        D::text(Face::Regular, 10, stackR.center().x, stackR.y + 26, "operands wait here", theme::textMute, Align::Center);
        for (int k = 0; k <= maxSlot; ++k) {
            float y = stackR.b() - 20 - (k + 1) * slotH;
            D::rect(Rect(stackR.x + 14, y, stackR.w - 28, 1), theme::violet.alpha(0.12f));
        }
        D::arrowHead(Vec2(stackR.r() + 16, stackR.b() - 26), Vec2(0, 1), 8, theme::violet.alpha(0.4f));
        D::text(Face::Regular, 10, stackR.r() + 6, stackR.b() - 18, "push", theme::textMute);
    }

    // ---------------------------------------------------------- edges
    for (int i = 0; i < nb; ++i) {
        const TreeNode& n = S.nodes[i];
        if (T < n.born) continue;
        float grow = easeOutCubic((T - n.born) / 0.5f);
        for (int ch : {n.left, n.right}) {
            if (ch < 0) continue;
            Vec2 a = posOf(i), b = posOf(ch);
            Vec2 p0 = a + Vec2(0, 20), p3 = b - Vec2(0, 20);
            Vec2 p1 = p0 + Vec2(0, (p3.y - p0.y) * 0.5f), p2 = p3 - Vec2(0, (p3.y - p0.y) * 0.5f);
            bool done = evalIdx >= ch;
            Color col = done ? theme::green.alpha(0.85f) : theme::textMute;
            partialBezier(p0, p1, p2, p3, grow, col, done ? 3.0f : 2.2f);
            // value flowing up from the child to this parent during code generation
            if (evalIdx == i) {
                float ft = clampf((evalT - evalIdx) / 0.6f, 0, 1);
                Vec2 dot = D::bezierPoint(p3, p2, p1, p0, ft);
                D::circle(dot, 9, theme::green.alpha(0.3f));
                D::circle(dot, 4.5f, Color(1, 1, 1, 1));
            }
        }
    }

    // ---------------------------------------------------------- nodes
    Vec2 currentPos(-1, -1);
    for (int i = 0; i < nb; ++i) {
        const TreeNode& n = S.nodes[i];
        if (T < n.born) continue;
        float s = easeOutBack((T - n.born) / 0.45f);
        Vec2 p = posOf(i);
        Color col = nodeColor(n.kind);
        bool evaluated = evalIdx >= i;
        bool current = (building && i == buildIdx) || evalIdx == i;
        if (current) currentPos = p;
        if (evaluated && !current) col = col.lerp(theme::green, 0.25f);
        if (n.kind == 0 || n.kind >= 3) {
            float rad = (n.kind >= 3 ? 30.f : 25.f) * s;
            if (current) D::glow(Rect(p.x - rad, p.y - rad, 2 * rad, 2 * rad), rad, 14, col);
            D::circle(p + Vec2(0, 3), rad, Color(0, 0, 0, 0.35f), 40);
            D::circle(p, rad, col.lerp(theme::panel, 0.35f), 40);
            D::circle(p - Vec2(0, rad * 0.15f), rad * 0.86f, col.lerp(theme::panel, 0.15f), 40);
            D::ring(p, rad, current ? 2.5f : 1.5f, col.lighten(0.3f), 40);
            D::text(Face::Bold, n.kind >= 3 ? 24 : 21, p.x, p.y - (n.kind >= 3 ? 15 : 13), n.label, Color(1, 1, 1, std::min(1.f, s)),
                    Align::Center);
        } else {
            float w = std::max(54.f, D::textWidth(Face::MonoBold, 17, n.label) + 26) * s, h = 38 * s;
            Rect r(p.x - w / 2, p.y - h / 2, w, h);
            if (current) D::glow(r, 11, 12, col);
            D::shadow(r, 11, 8, Color(0, 0, 0, 1));
            D::roundRectGrad(r, 11, col.lerp(theme::panel, 0.45f), col.lerp(theme::panel, 0.65f));
            D::roundRectOutline(r, 11, col.lighten(0.2f), current ? 2.f : 1.2f);
            D::textIn(Face::MonoBold, 17, r, n.label, Color(1, 1, 1, std::min(1.f, s)), Align::Center);
        }
        if (evaluated && !n.tag.empty()) {
            float tw = D::textWidth(Face::MonoBold, 12, n.tag) + 16;
            float off = n.kind == 0 || n.kind >= 3 ? 34.f : 26.f;
            Rect tr(p.x - tw / 2, p.y + off, tw, 20);
            Color tc = n.tag[0] == '[' ? theme::orange
                       : (std::isdigit(static_cast<unsigned char>(n.tag[0])) || n.tag[0] == '-') ? theme::amber
                                                                                                   : theme::green;
            D::roundRect(tr, 10, theme::overlay.alpha(0.95f));
            D::roundRectOutline(tr, 10, tc.alpha(0.8f));
            D::textIn(Face::MonoBold, 12, tr, n.tag, tc.lighten(0.3f), Align::Center);
        }
    }

    // ---------------------------------------------------------- code panel
    D::roundRect(codeR, 12, theme::overlay.alpha(0.95f));
    D::roundRectOutline(codeR, 12, theme::green.alpha(0.35f));
    D::text(Face::Bold, 11, codeR.x + 14, codeR.y + 12, "GENERATED x86-64", theme::textMute);
    std::vector<int> revealed, hot;
    for (int i = 0; i <= evalIdx && i < nb; ++i)
        for (int l : S.nodes[i].lines) {
            revealed.push_back(l);
            if (i == evalIdx) hot.push_back(l);
        }
    float ly = codeR.y + 38;
    Vec2 hotAnchor(-1, -1);
    for (int l : S.lines) {
        if (ly > codeR.b() - 22) break;
        bool on = std::find(revealed.begin(), revealed.end(), l) != revealed.end();
        bool isHot = std::find(hot.begin(), hot.end(), l) != hot.end();
        Rect lr(codeR.x + 8, ly - 3, codeR.w - 16, 24);
        if (isHot) {
            D::roundRect(lr, 6, theme::green.alpha(0.18f));
            D::glow(lr, 6, 5, theme::green.alpha(0.6f));
            if (hotAnchor.x < 0) hotAnchor = Vec2(lr.x, lr.center().y);
        }
        if (l >= 0 && l < int(R.code.size())) drawAsmLine(codeR.x + 14, ly, R.code[l], on ? 1.f : 0.2f, 13);
        ly += 26;
    }
    // connector from the active node to the instructions it produced
    if (!building && hotAnchor.x > 0 && currentPos.x > 0) {
        float k = clampf((evalT - evalIdx) / 0.5f, 0, 1);
        Vec2 a = currentPos + Vec2(30, 0);
        Vec2 b = hotAnchor - Vec2(4, 0);
        partialBezier(a, a + Vec2((b.x - a.x) * 0.5f, 0), b - Vec2((b.x - a.x) * 0.5f, 0), b, k, theme::green.alpha(0.55f), 1.6f);
        if (k >= 1) D::arrowHead(b, Vec2(1, 0), 7, theme::green.alpha(0.7f));
    }

    // ---------------------------------------------------------- caption bar
    Rect bar(vp.x + 16, vp.b() - 70, vp.w - 32, 56);
    D::roundRect(bar, 14, theme::overlay.alpha(0.95f));
    D::roundRectOutline(bar, 14, theme::border);
    float bx = bar.x + 10;
    if (ui.iconButton("psPrev", Rect(bx, bar.y + 11, 34, 34), Icon::ChevronL, "Previous statement")) {
        scene_ = (scene_ + int(scenes_.size()) - 1) % int(scenes_.size()); sceneT_ = 0; hold_ = 0;
    }
    if (ui.iconButton("psPlay", Rect(bx + 38, bar.y + 11, 34, 34), scenePlaying_ ? Icon::Pause : Icon::Play, "Play / pause"))
        scenePlaying_ = !scenePlaying_;
    if (ui.iconButton("psRestart", Rect(bx + 76, bar.y + 11, 34, 34), Icon::Reset, "Replay this statement")) { sceneT_ = 0; hold_ = 0; }
    if (ui.iconButton("psNext", Rect(bx + 114, bar.y + 11, 34, 34), Icon::ChevronR, "Next statement")) {
        scene_ = (scene_ + 1) % int(scenes_.size()); sceneT_ = 0; hold_ = 0;
    }
    float tx = bx + 162;
    D::text(Face::Semibold, 11, tx, bar.y + 9,
            "STATEMENT " + std::to_string(scene_ + 1) + " / " + std::to_string(scenes_.size()), theme::textMute);
    D::text(Face::MonoBold, 15, tx, bar.y + 26, D::ellipsize(Face::MonoBold, 15, S.title, 260), theme::text);
    float cx = tx + 280;
    ui.pill(Rect(cx, bar.y + 9, building ? 104 : 124, 20), building ? "1  BUILD TREE" : "2  GENERATE CODE",
            building ? theme::violet : theme::green, true, 10);
    std::string cap;
    if (building) {
        const TreeNode& n = S.nodes[buildIdx];
        if (n.kind == 1 || n.kind == 2) cap = n.note.empty() || buildIdx > 0 || n.kind == 2
                                                  ? "Read '" + n.label + "': an operand, so push it onto the stack"
                                                  : n.note;
        else if (n.kind == 0) cap = std::string("Read '") + n.label + "': an operator pops " +
                                    (n.right < 0 ? "one operand" : "two operands") + ", makes them its children and is pushed back";
        else cap = n.note;
    } else {
        cap = S.nodes[evalIdx].note.empty() ? "Post-order walk: children are evaluated before their parent"
                                            : S.nodes[evalIdx].note;
    }
    D::text(Face::Regular, 13, cx, bar.y + 32, D::ellipsize(Face::Regular, 13, cap, bar.r() - cx - 16), theme::textDim);
    Rect pb(cx + 140, bar.y + 16, bar.r() - cx - 156, 5);
    D::roundRect(pb, 2.5f, theme::bg1);
    D::roundRect(Rect(pb.x, pb.y, pb.w * (T / std::max(1.f, total)), pb.h), 2.5f, building ? theme::violet : theme::green);

    // legend (top right of the tree area)
    float lx = vp.x + 330;
    const std::pair<const char*, int> leg[] = {{"variable", 1}, {"number", 2}, {"operator", 0}, {"= / compare", 3}};
    for (const auto& l : leg) {
        D::circle(Vec2(lx + 6, vp.y + 31), 5, nodeColor(l.second));
        lx += D::text(Face::Regular, 12, lx + 16, vp.y + 24, l.first, theme::textDim) + 30;
    }
}

} // namespace vas::ui
