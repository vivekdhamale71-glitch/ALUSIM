// =====================================================================
//  Arithmetic view                                        [COA CO3]
//  Processor-level arithmetic shown bit by bit: Booth's multiplication,
//  add-and-shift multiplication, restoring division and number-system
//  conversions (hex <-> BCD). Operands can be taken from the last
//  IMUL / IDIV the CPU executed. Register bits slide during shifts
//  (translation animation).
// =====================================================================
#include <algorithm>
#include <cmath>
#include <cstdio>

#include "engine/gfx/Scene3D.h"
#include "app/Views.h"

namespace vas::ui {

using namespace gfx;
using namespace core;
namespace D = gfx::draw;

static std::int64_t bitsValue(const std::string& b, bool isSigned) {
    std::int64_t v = 0;
    for (char c : b) v = v * 2 + (c == '1');
    if (isSigned && !b.empty() && b[0] == '1') v -= (std::int64_t(1) << b.size());
    return v;
}

void ArithmeticView::draw(const Rect& area, Sim& sim, UI& ui) {
    Rect a = area;
    Rect right = a.cutRight(std::min(380.f, a.w * 0.32f));
    a.cutRight(theme::gap);

    // ------------------------------------------------------------- controls
    Rect top = a.cutTop(112);
    a.cutTop(theme::gap);
    Rect tc = ui.card(top, "COMPUTER ARITHMETIC", theme::amber, "how the ALU multiplies and divides");
    int nm = ui.segmented("arMode", Rect(tc.x, tc.y - 2, std::min(560.f, tc.w - 172), 34),
                          {"Booth's", "Add & Shift", "Restoring Division", "Hex \xE2\x86\x94 BCD"}, mode_, theme::amber);
    if (nm != mode_) { mode_ = nm; key_.clear(); }
    Rect row(tc.x, tc.y + 40, tc.w, 32);
    float x = row.x;
    if (mode_ == 0) {
        ui.stepper("ba", Rect(x, row.y, 210, 32), a_, -128, 127, "Multiplicand M");
        ui.stepper("bb", Rect(x + 226, row.y, 196, 32), b_, -128, 127, "Multiplier Q");
        x += 436;
    } else if (mode_ == 1) {
        ui.stepper("ua", Rect(x, row.y, 210, 32), ua_, 0, 255, "Multiplicand M");
        ui.stepper("ub", Rect(x + 226, row.y, 196, 32), ub_, 0, 255, "Multiplier Q");
        x += 436;
    } else if (mode_ == 2) {
        ui.stepper("dd", Rect(x, row.y, 190, 32), dividend_, 0, 255, "Dividend Q");
        ui.stepper("dv", Rect(x + 206, row.y, 180, 32), divisor_, 0, 255, "Divisor M");
        x += 400;
    } else {
        ui.stepper("nv", Rect(x, row.y, 250, 32), dividend_, 0, 9999, "Value");
        x += 266;
    }
    // take operands from the program that just ran
    const ExecRecord* lastMul = nullptr;
    const ExecRecord* lastDiv = nullptr;
    for (const auto& r : sim.trace) {
        if (r.aluOp == AluOp::Mul) lastMul = &r;
        if (r.aluOp == AluOp::Div) lastDiv = &r;
    }
    const ExecRecord* src = (mode_ == 2) ? lastDiv : (mode_ == 3 ? (sim.trace.empty() ? nullptr : &sim.trace.back()) : lastMul);
    std::string fromLabel = mode_ == 2 ? "From last IDIV" : mode_ == 3 ? "From RAX" : "From last IMUL";
    if (ui.button("arFrom", Rect(x, row.y, 150, 32), fromLabel, Style::Secondary, int(Icon::Chip), src != nullptr || mode_ == 3)) {
        if (mode_ == 0 && src) { a_ = int(std::max<std::int64_t>(-128, std::min<std::int64_t>(127, std::int64_t(src->aluA)))); b_ = int(std::max<std::int64_t>(-128, std::min<std::int64_t>(127, std::int64_t(src->aluB)))); }
        if (mode_ == 1 && src) { ua_ = int(src->aluA & 0xFF); ub_ = int(src->aluB & 0xFF); }
        if (mode_ == 2 && src) { dividend_ = int(src->aluA & 0xFF); divisor_ = int(src->aluB & 0xFF); }
        if (mode_ == 3) dividend_ = int(std::min<std::uint64_t>(9999, sim.shownRegs[RAX]));
        key_.clear();
    }
    if (ui.hovered(Rect(x, row.y, 150, 32)))
        ui.tooltip(src ? "Use the operands of the instruction the CPU executed" : "Run a program with * or / first");
    // playback
    float px = tc.r() - 4 * 38;
    if (ui.iconButton("arPlay", Rect(px, tc.y - 1, 34, 32), playing_ ? Icon::Pause : Icon::Play, "Play / pause")) playing_ = !playing_;
    if (ui.iconButton("arStep", Rect(px + 38, tc.y - 1, 34, 32), Icon::Step, "Next step")) { playing_ = false; ++shown_; t_ = 0; }
    if (ui.iconButton("arRestart", Rect(px + 76, tc.y - 1, 34, 32), Icon::Reset, "Restart")) { shown_ = 0; t_ = 0; playing_ = true; }
    if (ui.iconButton("arEnd", Rect(px + 114, tc.y - 1, 34, 32), Icon::Finish, "Show all steps")) { shown_ = 1000; playing_ = false; }

    // ------------------------------------------------------------- trace
    char kb[96];
    std::snprintf(kb, sizeof kb, "%d|%d|%d|%d|%d|%d|%d", mode_, a_, b_, ua_, ub_, dividend_, divisor_);
    if (key_ != kb) {
        key_ = kb;
        if (mode_ == 0) trace_ = boothMultiply(a_, b_, bits_);
        else if (mode_ == 1) trace_ = addShiftMultiply(unsigned(ua_), unsigned(ub_), bits_);
        else if (mode_ == 2) trace_ = restoringDivide(unsigned(dividend_), unsigned(divisor_), bits_);
        else trace_ = ArithTrace{};
        shown_ = 0;
        t_ = 0;
        playing_ = true;
    }

    if (mode_ == 3) {
        // ------------------------------------------------ number systems
        NumberViews nv = numberViews(dividend_);
        Rect c = ui.card(a, "NUMBER SYSTEMS  \xC2\xB7  BINARY  \xC2\xB7  HEX  \xC2\xB7  BCD", theme::amber);
        float y = c.y;
        auto rowLabel = [&](const std::string& l, const std::string& v, const Color& col) {
            D::text(Face::Bold, 11, c.x, y + 4, l, theme::textMute);
            D::text(Face::MonoBold, 20, c.x + 120, y, v, col);
            y += 38;
        };
        rowLabel("DECIMAL", nv.dec, theme::text);
        rowLabel("HEXADECIMAL", "0x" + nv.hex.substr(nv.hex.find_first_not_of('0') == std::string::npos ? 15 : nv.hex.find_first_not_of('0')), theme::cyan);
        // binary nibbles
        D::text(Face::Bold, 11, c.x, y + 8, "BINARY (16 bit)", theme::textMute);
        float cw = std::min(30.f, (c.w - 140) / 17.5f);
        for (int i = 0; i < 16; ++i) {
            int bit = (dividend_ >> (15 - i)) & 1;
            Rect br(c.x + 120 + i * cw + (i / 4) * 8, y, cw - 3, 30);
            D::roundRect(br, 5, bit ? theme::cyan.alpha(0.3f) : theme::bg1);
            D::roundRectOutline(br, 5, bit ? theme::cyan : theme::border);
            D::textIn(Face::MonoBold, 14, br, bit ? "1" : "0", bit ? theme::text : theme::textMute, Align::Center);
        }
        y += 52;
        // BCD digits
        D::text(Face::Bold, 11, c.x, y + 8, "PACKED BCD", theme::textMute);
        std::string digits = std::to_string(dividend_);
        float bx = c.x + 120;
        for (char d : digits) {
            int v = d - '0';
            Rect dr(bx, y, 4 * cw + 6, 30);
            D::roundRect(dr, 6, theme::amber.alpha(0.12f));
            D::roundRectOutline(dr, 6, theme::amber.alpha(0.5f));
            std::string nib = toBits(std::uint64_t(v), 4);
            D::textIn(Face::MonoBold, 14, dr, nib, theme::amber.lighten(0.3f), Align::Center);
            D::text(Face::Bold, 13, dr.center().x, y + 34, std::string(1, d), theme::text, Align::Center);
            bx += dr.w + 10;
        }
        y += 70;
        // conversion steps
        float colW = (c.w - 20) / 2;
        Rect l(c.x, y, colW, c.b() - y), rr(c.x + colW + 20, y, colW, c.b() - y);
        D::text(Face::Bold, 12, l.x, l.y, "HEX \xE2\x86\x92 BCD  (repeated division by 10)", theme::cyan);
        int v = dividend_;
        float yy = l.y + 26;
        std::vector<int> rem;
        do {
            int q = v / 10, r = v % 10;
            char b[96];
            std::snprintf(b, sizeof b, "0x%X / 10 = 0x%X   remainder %d", v, q, r);
            D::text(Face::Mono, 13, l.x, yy, b, theme::textDim);
            yy += 22;
            rem.push_back(r);
            v = q;
        } while (v > 0 && yy < l.b() - 20);
        std::string bcd;
        for (int i = int(rem.size()) - 1; i >= 0; --i) bcd += char('0' + rem[i]);
        D::text(Face::MonoBold, 13, l.x, yy + 4, "digits read bottom-up \xE2\x86\x92 BCD " + bcd, theme::green);
        D::text(Face::Bold, 12, rr.x, rr.y, "BCD \xE2\x86\x92 HEX  (acc = acc x 10 + digit)", theme::amber);
        yy = rr.y + 26;
        int acc = 0;
        for (char d : digits) {
            int prev = acc;
            acc = acc * 10 + (d - '0');
            char b[96];
            std::snprintf(b, sizeof b, "0x%X x 10 + %c = 0x%X", prev, d, acc);
            D::text(Face::Mono, 13, rr.x, yy, b, theme::textDim);
            yy += 22;
        }
        D::text(Face::MonoBold, 13, rr.x, yy + 4, nv.bcdBack, theme::green);

        Rect ec = ui.card(right, "WHY IT MATTERS", theme::cyan);
        D::paragraph(Face::Regular, 13, ec,
                     "The CPU stores numbers in binary, people read decimal. BCD keeps every decimal digit in its own "
                     "4-bit nibble, which makes printing easy but arithmetic harder.\n\nThe x86 family even has "
                     "instructions for packed BCD (DAA, DAS on the 8086/80386). Converting to BCD uses repeated "
                     "division by 10; converting back multiplies the accumulator by 10 and adds each digit - exactly "
                     "the loops written in COA practical 4.",
                     theme::textDim);
        return;
    }

    // timing
    int nSteps = int(trace_.steps.size());
    if (playing_ && shown_ < nSteps) {
        t_ += ui.dt * std::max(0.5f, sim.speed);
        if (t_ >= 0.9f) { t_ = 0; ++shown_; }
    }
    shown_ = std::min(shown_, nSteps);
    int curIdx = std::max(0, std::min(nSteps - 1, shown_ == 0 ? 0 : shown_ - 1));
    float anim = shown_ == 0 ? 1.f : easeOutCubic(t_ / 0.45f);
    if (!playing_ || shown_ >= nSteps) anim = 1.f;

    // ------------------------------------------------------- register bits
    Rect regs = a.cutTop(std::min(262.f, a.h * 0.42f));
    a.cutTop(theme::gap);
    Rect rc = ui.card(regs, "REGISTERS  \xC2\xB7  " + trace_.title, theme::cyan,
                      nSteps ? "step " + std::to_string(curIdx + 1) + " / " + std::to_string(nSteps) : "");
    if (!trace_.ok && nSteps == 0) {
        D::paragraph(Face::Regular, 14, rc, trace_.result, theme::red);
    } else if (nSteps > 0) {
        const ArithStep& st = trace_.steps[curIdx];
        const ArithStep* prev = curIdx > 0 ? &trace_.steps[curIdx - 1] : nullptr;
        // ---- 3D register cubes: a bit flips like a tile when it changes and the
        //      whole register slides one place during a shift          [CG CO2, CO3, CO6]
        int maxBits = 0;
        for (const auto& r : st.regs) maxBits += int(r.size());
        bool shiftRight = st.isShift && mode_ != 2;
        bool shiftLeft = st.isShift && mode_ == 2;
        const float pitchX = 1.0f, regGap = 0.9f, cube = 0.86f;
        float W = maxBits * pitchX + (st.regs.size() - 1) * regGap;
        Rect strip(rc.x - 8, rc.y + 18, rc.w + 16, 104);
        float aspect = strip.w / strip.h;
        s3d::Camera cam;
        cam.yaw = std::sin(ui.time * 0.3f) * 4;
        cam.pitch = 24;
        cam.fov = 22;
        cam.dist = std::max(8.f, W / (0.389f * aspect) * 1.2f);
        cam.ty = 0.1f;
        s3d::begin(strip, cam, theme::light);
        struct BitLabel { Vec2 p; bool one; bool hot; Color col; };
        struct RegLabel { Vec2 p; std::string name; std::string value; Color col; };
        std::vector<BitLabel> bitLabels;
        std::vector<RegLabel> regLabels;
        float x = -W / 2 + pitchX / 2;
        for (std::size_t k = 0; k < st.regs.size(); ++k) {
            const std::string& bits = st.regs[k];
            const std::string& name = trace_.columns[k];
            bool isM = name == "M";
            Color col = isM ? theme::violet : (name == "A" ? theme::cyan : (name == "Q" ? theme::amber : theme::pink));
            bool sgn = mode_ == 0 && (name == "A" || name == "Q" || name == "M");
            RegLabel rl;
            s3d::project(x - pitchX * 0.45f, 0.75f, 0, rl.p);
            rl.name = name;
            rl.value = bits.size() > 1 ? "= " + std::to_string(bitsValue(bits, sgn)) : "";
            rl.col = col;
            regLabels.push_back(rl);
            // register tray
            s3d::material(theme::panel3, 0.0f, 10);
            s3d::box(x - pitchX * 0.5f, -0.62f, -0.6f, x + (bits.size() - 0.5f) * pitchX, -0.5f, 0.6f);
            for (int i = 0; i < int(bits.size()); ++i) {
                bool one = bits[i] == '1';
                bool changed = prev && k < prev->regs.size() && prev->regs[k].size() == bits.size() && prev->regs[k][i] != bits[i];
                float off = 0;
                if (!isM && (shiftRight || shiftLeft)) off = (1 - anim) * pitchX * (shiftRight ? -1 : 1);
                float flip = (changed && !st.isShift) ? 180.f * (1 - anim) : 0.f;
                float bxp = x + i * pitchX + off;
                Color c = one ? col : theme::panel3.lerp(theme::textMute, 0.25f);
                s3d::material(c, one ? (changed && anim < 1 ? 0.45f : 0.12f) : 0.0f, one ? 70.f : 15.f);
                s3d::boxRotX(bxp, 0, 0, cube, cube, cube, flip);
                BitLabel bl;
                s3d::project(bxp, 0, cube / 2 + 0.01f, bl.p);
                bl.one = one;
                bl.hot = changed && anim < 1;
                bl.col = col;
                if (flip < 90) bitLabels.push_back(bl);
            }
            x += bits.size() * pitchX + regGap;
        }
        s3d::end();
        for (const auto& b : bitLabels)
            D::text(Face::MonoBold, 16, b.p.x, b.p.y - 11, b.one ? "1" : "0",
                    b.one ? theme::onAccent : theme::textMute, Align::Center);
        for (const auto& r : regLabels) {
            float w = D::text(Face::Bold, 13, r.p.x, rc.y - 2, r.name, r.col);
            if (!r.value.empty()) D::text(Face::Mono, 11, r.p.x + w + 6, rc.y, r.value, theme::textMute);
        }
        float by = strip.b() - 64;
        Rect act(rc.x, by + 70, rc.w, 34);
        Color ac = st.isShift ? theme::blue : (st.action.find("no ") != std::string::npos ? theme::textDim : theme::green);
        D::roundRect(act, 9, ac.alpha(0.12f));
        D::roundRectOutline(act, 9, ac.alpha(0.5f));
        D::icon(st.isShift ? (mode_ == 2 ? Icon::ChevronL : Icon::ChevronR) : Icon::Plus, Rect(act.x + 10, act.y + 9, 16, 16), ac);
        D::textIn(Face::Semibold, 14, act, "cycle " + st.cycle + "   " + st.action, theme::text, Align::Left, 34);
        if (shown_ >= nSteps) {
            Rect res(rc.x, act.b() + 10, rc.w, 32);
            D::roundRect(res, 9, (trace_.ok ? theme::green : theme::red).alpha(0.14f));
            D::icon(trace_.ok ? Icon::Check : Icon::Cross, Rect(res.x + 10, res.y + 8, 16, 16), trace_.ok ? theme::green : theme::red);
            D::textIn(Face::MonoBold, 13, res, D::ellipsize(Face::MonoBold, 13, trace_.result, res.w - 50), theme::text, Align::Left, 34);
        }
    }

    // ------------------------------------------------------------ step table
    Rect tcard = ui.card(a, "STEP TABLE", theme::violet, std::to_string(std::min(shown_, nSteps)) + " of " + std::to_string(nSteps));
    float colX[8];
    std::vector<std::string> head = {"Cycle"};
    for (const auto& c : trace_.columns) head.push_back(c);
    head.push_back("Operation");
    float cx = tcard.x;
    for (std::size_t k = 0; k < head.size(); ++k) {
        colX[k] = cx;
        float w = 60;
        if (k > 0 && k < head.size() - 1) w = std::max(48.f, trace_.steps.empty() ? 60.f : float(trace_.steps[0].regs[k - 1].size()) * 10.5f + 24);
        D::text(Face::Bold, 11, cx, tcard.y - 2, head[k], theme::textMute);
        cx += w;
    }
    float rowH = 24;
    Rect body(tcard.x - 4, tcard.y + 18, tcard.w + 8, tcard.h - 18);
    int visible = int(body.h / rowH);
    float scroll = ui.anim("arScroll", float(std::max(0, std::min(shown_, nSteps) - visible)), 8);
    D::pushClip(body);
    for (int i = 0; i < std::min(shown_, nSteps); ++i) {
        const ArithStep& st = trace_.steps[i];
        float y = body.y + (i - scroll) * rowH;
        if (y + rowH < body.y || y > body.b()) continue;
        bool cur = i == curIdx;
        Rect rr(body.x, y, body.w, rowH - 2);
        if (cur) { D::roundRect(rr, 5, theme::violet.alpha(0.2f)); D::rect(Rect(rr.x, rr.y + 3, 3, rr.h - 6), theme::violet); }
        else if (i % 2) D::rect(rr, Color(1, 1, 1, 0.02f));
        float ty = y + 4;
        D::text(Face::MonoBold, 12, colX[0] + 4, ty, st.cycle, theme::textDim);
        for (std::size_t k = 0; k < st.regs.size(); ++k)
            D::text(Face::Mono, 13, colX[k + 1], ty, st.regs[k], st.isShift ? theme::blue : theme::text);
        D::text(Face::Regular, 12, colX[head.size() - 1], ty, st.action, st.isShift ? theme::blue : theme::textDim);
    }
    D::popClip();

    // --------------------------------------------------------- explanation
    Rect ec = ui.card(right, "HOW IT WORKS", theme::cyan);
    float h = D::paragraph(Face::Regular, 13, Rect(ec.x, ec.y, ec.w, 200), trace_.summary, theme::textDim);
    float y = ec.y + h + 12;
    if (mode_ == 0) {
        D::text(Face::Bold, 11, ec.x, y, "BOOTH RECODING TABLE", theme::textMute);
        y += 20;
        const char* rowsT[4][2] = {{"0  0", "shift only"}, {"0  1", "A = A + M, shift"}, {"1  0", "A = A - M, shift"}, {"1  1", "shift only"}};
        std::string curPair;
        if (nSteps && curIdx < nSteps) {
            const auto& st = trace_.steps[curIdx];
            if (st.action.size() > 12 && st.action.rfind("Q0 Q-1 = ", 0) == 0)
                curPair = std::string(1, st.action[9]) + "  " + st.action[10];
        }
        for (auto& r : rowsT) {
            Rect rr(ec.x, y, ec.w, 28);
            bool sel = curPair == r[0];
            D::roundRect(rr, 7, sel ? theme::amber.alpha(0.2f) : theme::bg1);
            if (sel) D::roundRectOutline(rr, 7, theme::amber.alpha(0.7f));
            D::text(Face::MonoBold, 13, rr.x + 12, rr.y + 6, r[0], sel ? theme::amber : theme::text);
            D::text(Face::Regular, 13, rr.x + 70, rr.y + 6, r[1], theme::textDim);
            y += 32;
        }
        y += 8;
        D::paragraph(Face::Regular, 12, Rect(ec.x, y, ec.w, 120),
                     "Booth's algorithm skips over runs of 1s in the multiplier, so it needs fewer additions than "
                     "plain add-and-shift and works directly on two's-complement (signed) numbers - the same idea "
                     "used inside the hardware multiplier behind IMUL.",
                     theme::textMute);
    } else if (mode_ == 1) {
        D::paragraph(Face::Regular, 12, Rect(ec.x, y, ec.w, 200),
                     "This is long multiplication in binary. For every bit of the multiplier the multiplicand is "
                     "either added (bit = 1) or not (bit = 0), then everything shifts right one place. After n "
                     "cycles the 2n-bit product sits in A:Q.\n\nIt is exactly COA practical 5 (multiplication by add "
                     "and shift) done by the hardware, one clock cycle per bit.",
                     theme::textMute);
    } else {
        D::paragraph(Face::Regular, 12, Rect(ec.x, y, ec.w, 200),
                     "Restoring division is binary long division. Each cycle shifts A:Q left, tries A - M and looks "
                     "at the sign: a negative result means M did not fit, so A is restored and the quotient bit is 0.\n\n"
                     "Dividing by zero is impossible - the x86 CPU raises a divide error (#DE), which this simulator "
                     "models as a DivideByZeroError exception.",
                     theme::textMute);
    }
}

} // namespace vas::ui
