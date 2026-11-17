// =====================================================================
//  Compiler view - the statement -> assembly animation
//  Plays the CompileResult timeline event by event:
//    lexer tokens  ->  shunting-yard (stack + queue)  ->  code generation
//    (value stack, emitted instructions)  ->  hash symbol table
// =====================================================================
#include <algorithm>
#include <cmath>
#include <cstdio>

#include "PSOOP_2310261L/CO3_ProblemSolving/Compiler.h"
#include "app/Samples.h"
#include "app/Views.h"

namespace vas::ui {

using namespace gfx;
using core::TimelineEvent;
using core::TokType;
namespace D = gfx::draw;

static Color tokColor(TokType t) {
    switch (t) {
        case TokType::Number: return theme::amber;
        case TokType::Ident: return theme::ident;
        case TokType::Keyword: return theme::violet;
        case TokType::Op: return theme::pink;
        case TokType::RelOp: return theme::orange;
        case TokType::Assign: return theme::teal;
        default: return theme::textDim;
    }
}

static float eventDuration(const TimelineEvent& e) {
    switch (e.kind) {
        case TimelineEvent::Lex: return 0.16f;
        case TimelineEvent::StmtBegin: return 0.6f;
        case TimelineEvent::Shunt: return 0.7f;
        case TimelineEvent::Gen: return 0.9f;
        case TimelineEvent::Symbol: return 1.3f;
        case TimelineEvent::Emit: return 0.8f;
        case TimelineEvent::Done: return 0.5f;
    }
    return 0.5f;
}

static const char* eventStage(const TimelineEvent& e) {
    switch (e.kind) {
        case TimelineEvent::Lex: return "LEXER";
        case TimelineEvent::StmtBegin: return "PARSER";
        case TimelineEvent::Shunt: return "SHUNTING-YARD";
        case TimelineEvent::Gen: return "CODE GEN";
        case TimelineEvent::Symbol: return "SYMBOL TABLE";
        case TimelineEvent::Emit: return "EMIT";
        case TimelineEvent::Done: return "DONE";
    }
    return "";
}

static Color eventColor(const TimelineEvent& e) {
    switch (e.kind) {
        case TimelineEvent::Lex: return theme::cyan;
        case TimelineEvent::StmtBegin: return theme::violet;
        case TimelineEvent::Shunt: return theme::violet;
        case TimelineEvent::Gen: return theme::amber;
        case TimelineEvent::Symbol: return theme::pink;
        case TimelineEvent::Emit: return theme::green;
        case TimelineEvent::Done: return theme::green;
    }
    return theme::cyan;
}

static constexpr float kSentenceTime = 1.6f;   // seconds per sentence in the translation animation

static Color roleColor(core::WordRole r) {
    switch (r) {
        case core::WordRole::Verb: return theme::violet;
        case core::WordRole::Number: return theme::amber;
        case core::WordRole::Variable: return theme::ident;
        case core::WordRole::Operator: return theme::pink;
        case core::WordRole::Target: return theme::green;
        case core::WordRole::Keyword: return theme::textDim;
        default: return theme::textMute;
    }
}

// one generated statement line, syntax coloured
static void drawStmtLine(float x, float y, const std::string& line, float alpha, int px) {
    static Highlighter hl = statementHighlighter();
    Font& mono = Fonts::get(Face::Mono, px);
    float cw = mono.width("M");
    int pos = 0;
    for (const auto& sp : hl(line)) {
        if (sp.start > pos) mono.draw(x + pos * cw, y, line.substr(pos, sp.start - pos), theme::text.alpha(alpha));
        mono.draw(x + sp.start * cw, y, line.substr(sp.start, sp.end - sp.start), sp.color.alpha(alpha));
        pos = std::max(pos, sp.end);
    }
    if (pos < int(line.size())) mono.draw(x + pos * cw, y, line.substr(pos), theme::text.alpha(alpha));
}

// token pill size
static float pillWidth(const std::string& s, int px = 14) { return D::textWidth(Face::MonoBold, px, s) + 20; }

static void drawPill(const Rect& r, const std::string& s, const Color& c, float alpha = 1.f, bool hot = false, int px = 14) {
    if (hot) D::glow(r, r.h / 2, 8, c.alpha(alpha));
    D::roundRectGrad(r, 8, c.alpha(0.24f * alpha), c.alpha(0.12f * alpha));
    D::roundRectOutline(r, 8, c.alpha((hot ? 0.95f : 0.55f) * alpha), hot ? 1.5f : 1.f);
    D::textIn(Face::MonoBold, px, r, s, c.lighten(0.35f).alpha(alpha), Align::Center);
}

void CompileView::draw(const Rect& area, Sim& sim, UI& ui) {
    const core::CompileResult& R = sim.compiled;
    if (seenGen_ != sim.compileGen) {
        seenGen_ = sim.compileGen;
        ev_ = 0;
        evTime_ = 0;
        playing_ = true;
        tokenBorn_.assign(R.lex.size(), -1.f);
        transT_ = 0;
    }
    int nEv = int(R.timeline.size());
    // English mode: first play the sentence -> code translation, then compile
    bool english = sim.inputMode == MODE_ENGLISH;
    float transDur = english ? float(sim.english.sentences.size()) * kSentenceTime + 0.4f : 0.f;
    bool translating = english && transT_ < transDur && playing_;
    if (translating) transT_ += ui.dt * speed_;
    if (playing_ && !translating && nEv > 0 && ev_ < nEv - 1) {
        evTime_ += ui.dt * speed_;
        while (ev_ < nEv - 1 && evTime_ >= eventDuration(R.timeline[ev_])) {
            evTime_ -= eventDuration(R.timeline[ev_]);
            ++ev_;
        }
    }
    if (ev_ >= nEv - 1 && nEv > 0) { ev_ = nEv - 1; }

    Rect a = area;
    Rect bottom = a.cutBottom(54);
    a.cutBottom(theme::gap);
    float lw = std::floor(a.w * 0.38f);
    float mw = std::floor((a.w - lw) * 0.5f) - theme::gap;
    Rect left = a.cutLeft(lw);
    a.cutLeft(theme::gap);
    Rect mid = a.cutLeft(mw);
    a.cutLeft(theme::gap);
    Rect right = a;

    // ---------------------------------------------------------- editor card
    Rect edCard = left.cutTop(std::floor(left.h * 0.56f));
    left.cutTop(theme::gap);
    const char* modeSub[3] = {"plain English sentences", "C++ source code", "x86-64 assembly"};
    Rect ec = ui.card(edCard, "SOURCE", theme::cyan, modeSub[sim.inputMode]);
    Rect tb = ec.cutTop(34);
    ec.cutTop(8);
    Rect sampleR = ec.cutTop(32);
    ec.cutTop(8);
    int mode = ui.segmented("srcmode", Rect(tb.x, tb.y, std::min(270.f, tb.w - 160), 34), {"English", "C++", "Assembly"},
                            sim.inputMode);
    if (mode != sim.inputMode) {
        // pick the first sample of that kind so the editor is never empty
        const auto& s = samples();
        for (int i = 0; i < int(s.size()); ++i)
            if (s[i].mode == mode) { sim.loadSample(i); break; }
        sim.compile();
    }
    Rect compileBtn(tb.r() - 150, tb.y, 150, 34);
    if (ui.button("compile", compileBtn, "Compile & Load", Style::Primary, int(Icon::Bolt))) sim.compile();
    if (ui.hovered(compileBtn)) ui.tooltip("Ctrl+Enter");
    D::roundRect(sampleR, 9, theme::bg1);
    D::roundRectOutline(sampleR, 9, theme::border);
    if (ui.iconButton("prevSample", Rect(sampleR.x + 2, sampleR.y + 2, 28, 28), Icon::ChevronL, "Previous example")) {
        sim.loadSample(sim.sampleIndex - 1);
        sim.compile();
    }
    if (ui.iconButton("nextSample", Rect(sampleR.r() - 30, sampleR.y + 2, 28, 28), Icon::ChevronR, "Next example")) {
        sim.loadSample(sim.sampleIndex + 1);
        sim.compile();
    }
    {
        std::string label = "Example " + std::to_string(sim.sampleIndex + 1) + "/" + std::to_string(samples().size()) +
                            "   " + samples()[sim.sampleIndex].name;
        D::textIn(Face::Semibold, 12, sampleR.inset(34, 0), D::ellipsize(Face::Semibold, 12, label, sampleR.w - 70),
                  theme::textDim, Align::Center);
    }

    int activeLine = -1;
    if (sim.asmMode && sim.hasCurrent) activeLine = sim.currentLine();
    ui.codeEditor("editor", ec, sim.source, editor_,
                  sim.asmMode ? asmHighlighter() : (english ? englishHighlighter() : statementHighlighter()), activeLine,
                  sim.errorLine);

    // highlight the token / statement being processed inside the editor
    if (english && translating) {
        // the sentence being translated
        int si = std::min(int(sim.english.sentences.size()) - 1, int(transT_ / kSentenceTime));
        if (si >= 0) {
            int line = sim.english.sentences[si].line;
            Font& mono = Fonts::get(Face::Mono, 15);
            float lh = std::round(mono.height() * 1.55f);
            Rect hr(ec.x + 50, ec.y + 10 - editor_.scroll + line * lh + 1, ec.w - 56, lh - 2);
            float pulse = 0.5f + 0.5f * std::sin(ui.time * 8);
            D::pushClip(ec);
            D::roundRect(hr, 5, theme::violet.alpha(0.12f + 0.08f * pulse));
            D::roundRectOutline(hr, 5, theme::violet.alpha(0.7f));
            D::popClip();
        }
    }
    if (sim.inputMode == MODE_CPP && nEv > 0 && R.ok) {
        const TimelineEvent& e = R.timeline[ev_];
        int s0 = -1, s1 = -1;
        if (e.kind == TimelineEvent::Lex) { s0 = R.lex[e.a].tok.start; s1 = R.lex[e.a].tok.end; }
        else if (e.kind == TimelineEvent::StmtBegin) { s0 = R.stmts[e.a].start; s1 = R.stmts[e.a].end; }
        if (s0 >= 0 && ev_ < nEv - 1) {
            Font& mono = Fonts::get(Face::Mono, 15);
            float cw = mono.width("M"), lh = std::round(mono.height() * 1.55f);
            int line = 0, col = 0;
            for (int i = 0; i < s0 && i < int(sim.source.size()); ++i) {
                if (sim.source[i] == '\n') { ++line; col = 0; } else ++col;
            }
            int len = s1 - s0;
            bool multi = sim.source.substr(s0, len).find('\n') != std::string::npos;
            if (multi) len = int(sim.source.find('\n', s0) - s0);
            Rect hr(ec.x + 46 + 10 + col * cw - 3, ec.y + 10 - editor_.scroll + line * lh + 2, len * cw + 6, lh - 4);
            float pulse = 0.5f + 0.5f * std::sin(ui.time * 8);
            Color c = e.kind == TimelineEvent::Lex ? theme::cyan : theme::violet;
            D::pushClip(ec);
            D::roundRect(hr, 5, c.alpha(0.16f + 0.08f * pulse));
            D::roundRectOutline(hr, 5, c.alpha(0.8f));
            D::popClip();
        }
    }

    // ---------------------------------------------------------- tokens card
    if (english) drawEnglish(left, sim, ui);
    else if (!sim.asmMode) drawTokens(left, sim, ui, ev_);
    else {
        Rect tc = ui.card(left, "ASSEMBLER  \xC2\xB7  PASS 1 LABELS", theme::green);
        if (!sim.program) ui.emptyState(tc, "Not assembled", "Fix the error shown in red, then press Compile.");
        else {
            float y = tc.y;
            D::paragraph(Face::Regular, 12, Rect(tc.x, y, tc.w, 40),
                         "Pass 1 gives every label the index of the next instruction and stores it in a hash table. "
                         "Pass 2 looks each mnemonic up in the sorted opcode table with binary search.",
                         theme::textDim);
            y += 44;
            for (const auto& l : sim.program->labels) {
                D::text(Face::MonoBold, 14, tc.x + 4, y, l.first + ":", theme::green);
                D::text(Face::Mono, 13, tc.x + 150, y + 1, "instruction #" + std::to_string(l.second) + "   @ " +
                                                                  hex64(core::CPU::addressOf(l.second)),
                        theme::textDim);
                y += 24;
            }
            if (sim.program->labels.empty()) D::text(Face::Regular, 13, tc.x + 4, y, "(no labels)", theme::textMute);
        }
    }

    // ---------------------------------------------------------- middle
    bool englishFailed = english && !sim.english.ok;
    if (!sim.asmMode && (englishFailed || !R.ok) && sim.compileGen > 0) {
        Rect ecard = ui.card(mid, englishFailed ? "COULD NOT UNDERSTAND" : "COMPILE ERROR", theme::red);
        std::string kind = englishFailed ? "English parse error" : R.errorKind;
        std::string msg = englishFailed ? sim.english.error : R.error;
        int eline = englishFailed ? sim.english.errorLine : R.errorLine;
        D::roundRect(Rect(ecard.x, ecard.y, ecard.w, 120), 12, theme::red.alpha(0.1f));
        D::roundRectOutline(Rect(ecard.x, ecard.y, ecard.w, 120), 12, theme::red.alpha(0.5f));
        D::icon(Icon::Cross, Rect(ecard.x + 14, ecard.y + 16, 22, 22), theme::red);
        D::text(Face::Bold, 16, ecard.x + 46, ecard.y + 16, kind, theme::red.lighten(0.3f));
        D::paragraph(Face::Regular, 14, Rect(ecard.x + 46, ecard.y + 44, ecard.w - 60, 70), msg, theme::text);
        if (eline >= 0)
            D::text(Face::Semibold, 12, ecard.x + 46, ecard.y + 96, "line " + std::to_string(eline + 1), theme::textMute);
        const char* help = english
            ? "Sentences the translator understands:\n"
              "add 12 and 3   \xC2\xB7   add 5 to x   \xC2\xB7   subtract 4 from total\n"
              "multiply a by b   \xC2\xB7   divide 100 by 7 giving q\n"
              "set x to 5 plus y times 2   \xC2\xB7   x is 10\n"
              "increase count by 3   \xC2\xB7   double x   \xC2\xB7   swap a and b\n"
              "if x is greater than y then ... otherwise ...\n"
              "repeat 5 times: ...   \xC2\xB7   while i is less than 10 double i\n"
              "find the factorial of 5   \xC2\xB7   find the largest of a and b and c\n"
              "find the sum of numbers from 1 to 10   \xC2\xB7   raise 2 to the power of 8"
            : "The compiler raised a SyntaxError (an exception class). It was caught and shown here instead of "
              "crashing the program.\n\nSupported C++:  int x = e;   x += e;   x++;   + - * / % & | ^ << >> ~\n"
              "if (a > b) { ... } else { ... }    while (i < n) { ... }\n"
              "for (int i = 0; i < n; i++) { ... }    int main() { ... }";
        D::paragraph(Face::Regular, 13, Rect(ecard.x, ecard.y + 140, ecard.w, ecard.h - 140), help, theme::textDim);
    } else if (sim.asmMode) {
        Rect oc = ui.card(mid, "OPCODE LOOKUP  \xC2\xB7  BINARY SEARCH", theme::violet, "sorted with quick sort");
        const auto& tab = core::OpcodeTable::instance().sorted();
        if (sim.program && !sim.program->opcodeLookups.empty()) {
            int which = 0;
            if (sim.hasCurrent) which = sim.current.index;
            else which = int(ui.time / 1.6f) % int(sim.program->opcodeLookups.size());
            which = std::min(which, int(sim.program->opcodeLookups.size()) - 1);
            const auto& look = sim.program->opcodeLookups[which];
            D::text(Face::Semibold, 13, oc.x, oc.y, "Looking up", theme::textDim);
            D::text(Face::MonoBold, 18, oc.x + 82, oc.y - 3, look.mnemonic, theme::violet.lighten(0.3f));
            D::text(Face::Regular, 12, oc.r(), oc.y + 2,
                    std::to_string(look.probes.size()) + " probes for " + std::to_string(tab.size()) + " opcodes",
                    theme::textMute, Align::Right);
            float rowH = std::min(19.f, (oc.h - 34) / ((tab.size() + 1) / 2));
            float colW = oc.w / 2;
            for (int i = 0; i < int(tab.size()); ++i) {
                int col = i / int((tab.size() + 1) / 2), row = i % int((tab.size() + 1) / 2);
                Rect r(oc.x + col * colW, oc.y + 30 + row * rowH, colW - 8, rowH - 2);
                int probeOrder = -1;
                for (int k = 0; k < int(look.probes.size()); ++k) if (look.probes[k] == i) probeOrder = k;
                bool hit = probeOrder == int(look.probes.size()) - 1 && probeOrder >= 0;
                if (probeOrder >= 0) {
                    D::roundRect(r, 5, (hit ? theme::green : theme::violet).alpha(hit ? 0.3f : 0.16f));
                    D::text(Face::Bold, 10, r.r() - 6, r.y + 3, "#" + std::to_string(probeOrder + 1),
                            hit ? theme::green : theme::violet, Align::Right);
                }
                D::text(Face::MonoBold, 12, r.x + 6, r.y + 2, tab[i].mnemonic, probeOrder >= 0 ? theme::text : theme::textDim);
                D::text(Face::Mono, 10, r.x + 52, r.y + 4, tab[i].encoding, theme::textMute);
            }
        } else {
            ui.emptyState(oc, "No program", "Assemble a program to see the opcode lookups.");
        }
    } else {
        Rect pr = mid.cutTop(std::floor(mid.h * 0.55f));
        mid.cutTop(theme::gap);
        drawParser(pr, sim, ui, ev_);
        // code generation card is drawn inside drawParser's sibling
        Rect gc = ui.card(mid, "CODE GENERATION", theme::amber, "postfix \xE2\x86\x92 x86-64");
        int g = -1, gstep = -1;
        for (int i = 0; i <= ev_ && i < nEv; ++i)
            if (R.timeline[i].kind == TimelineEvent::Gen) { g = R.timeline[i].a; gstep = R.timeline[i].b; }
        if (g < 0) {
            ui.emptyState(gc, "Waiting for postfix", "Each postfix item is turned into instructions using a stack of values.");
        } else {
            const core::ExprTrace& ex = R.exprs[g];
            const core::GenStep& gs = ex.gen[gstep];
            bool hot = R.timeline[ev_].kind == TimelineEvent::Gen;
            float t = hot ? easeOutCubic(evTime_ / 0.35f) : 1.f;
            D::text(Face::Bold, 11, gc.x, gc.y, "POSTFIX", theme::textMute);
            float x = gc.x;
            for (int i = 0; i < int(ex.postfix.size()); ++i) {
                float w = pillWidth(ex.postfix[i], 13);
                Rect pr2(x, gc.y + 18, w, 28);
                bool cur = i == gs.postfixIndex;
                float al = i < gs.postfixIndex ? 0.35f : 1.f;
                drawPill(pr2, ex.postfix[i], cur ? theme::amber : theme::textDim, al, cur, 13);
                if (cur) D::arrowHead(Vec2(pr2.center().x, pr2.b() + 9), Vec2(0, -1), 7, theme::amber);
                x += w + 6;
            }
            D::text(Face::Bold, 11, gc.x, gc.y + 64, "VALUE STACK", theme::textMute);
            x = gc.x;
            for (int i = 0; i < int(gs.valueStack.size()); ++i) {
                const std::string& v = gs.valueStack[i];
                Color c = v[0] == '[' ? theme::orange : (std::isdigit(static_cast<unsigned char>(v[0])) || v[0] == '-')
                                                            ? theme::amber
                                                            : theme::ident;
                float w = pillWidth(v, 13);
                bool top = i == int(gs.valueStack.size()) - 1;
                float yoff = top ? (1 - t) * -18 : 0;
                drawPill(Rect(x, gc.y + 82 + yoff, w, 28), v, c, top ? t : 1.f, top && hot, 13);
                x += w + 6;
            }
            float y = gc.y + 124;
            if (!gs.lines.empty()) {
                D::text(Face::Bold, 11, gc.x, y, "EMITTED", theme::textMute);
                y += 18;
                for (int ln : gs.lines) {
                    Rect lr(gc.x, y, gc.w, 26);
                    D::roundRect(lr, 7, theme::green.alpha(0.08f + (hot ? 0.1f * (1 - t) : 0)));
                    drawAsmLine(lr.x + 8, lr.y + 4, R.code[ln], t, 14);
                    y += 29;
                }
            }
            D::paragraph(Face::Regular, 13, Rect(gc.x, std::max(y + 6, gc.b() - 52), gc.w, 52), gs.note, theme::textDim);
        }
    }

    // ---------------------------------------------------------- right
    Rect asmR = right.cutTop(std::floor(right.h * 0.58f));
    right.cutTop(theme::gap);
    drawAssembly(asmR, sim, ui, ev_);
    drawSymbols(right, sim, ui, ev_);

    // ---------------------------------------------------------- timeline bar
    D::shadow(bottom, 14, 14, Color(0, 0, 0, 1));
    D::roundRectGrad(bottom, 14, theme::panel.lighten(0.03f), theme::panel);
    D::roundRectOutline(bottom, 14, theme::border);
    Rect bb = bottom.inset(10, 8);
    if (ui.iconButton("cplay", Rect(bb.x, bb.y, 38, 38), playing_ && ev_ < nEv - 1 ? Icon::Pause : Icon::Play,
                      "Play / pause the compile animation", Style::Secondary)) {
        if (ev_ >= nEv - 1) { ev_ = 0; evTime_ = 0; playing_ = true; }
        else playing_ = !playing_;
    }
    if (ui.iconButton("crestart", Rect(bb.x + 44, bb.y, 38, 38), Icon::Reset, "Replay from the start")) {
        ev_ = 0;
        evTime_ = 0;
        playing_ = true;
    }
    if (ui.iconButton("cstep", Rect(bb.x + 88, bb.y, 38, 38), Icon::Step, "Next event")) {
        playing_ = false;
        if (ev_ < nEv - 1) { ++ev_; evTime_ = 0; }
    }
    if (ui.iconButton("cend", Rect(bb.x + 132, bb.y, 38, 38), Icon::Finish, "Skip to the end")) {
        ev_ = std::max(0, nEv - 1);
        evTime_ = 0;
    }
    Rect sp(bb.x + 186, bb.y + 6, 110, 26);
    ui.slider("cspeed", sp, speed_, 0.25f, 4.f);
    char sb[16];
    std::snprintf(sb, sizeof sb, "%.1fx", speed_);
    D::text(Face::MonoBold, 12, sp.r() + 10, bb.y + 11, sb, theme::textDim);

    Rect prog(bb.x + 350, bb.y + 4, bb.w - 350 - 330, 30);
    if (nEv > 0) {
        // event markers coloured by stage
        float segW = prog.w / nEv;
        for (int i = 0; i < nEv; ++i) {
            Color c = eventColor(R.timeline[i]);
            Rect s(prog.x + i * segW, prog.y + 18, std::max(1.f, segW - 1), 6);
            D::rect(s, c.alpha(i <= ev_ ? 0.85f : 0.18f));
        }
        float px = prog.x + (ev_ + std::min(1.f, evTime_ / eventDuration(R.timeline[ev_]))) * segW;
        D::circle(Vec2(px, prog.y + 21), 6, theme::text);
        D::circle(Vec2(px, prog.y + 21), 11, theme::text.alpha(0.15f));
        Rect hit(prog.x, prog.y, prog.w, prog.h);
        if (ui.hovered(hit) && ui.in.down) {
            ev_ = std::max(0, std::min(nEv - 1, int((ui.in.mouse.x - prog.x) / segW)));
            evTime_ = 0;
            playing_ = false;
        }
        const TimelineEvent& e = R.timeline[ev_];
        Color c = translating ? theme::violet : eventColor(e);
        const char* stage = translating ? "ENGLISH \xE2\x86\x92 CODE" : eventStage(e);
        float bw = D::textWidth(Face::Bold, 10, stage) + 16;
        ui.pill(Rect(prog.x, prog.y - 2, bw, 18), stage, c, true, 10);
        std::string cap;
        if (translating) {
            int si = std::min(int(sim.english.sentences.size()) - 1, int(transT_ / kSentenceTime));
            const auto& s = sim.english.sentences[std::max(0, si)];
            cap = "\"" + s.text + "\"  matches the rule  " + s.rule;
        } else
        switch (e.kind) {
            case TimelineEvent::Lex: {
                const auto& lx = R.lex[e.a];
                cap = "token '" + lx.tok.text + "'  \xE2\x86\x92  " + core::tokTypeName(lx.tok.type);
                break;
            }
            case TimelineEvent::StmtBegin: cap = R.stmts[e.a].kind + ":  " + R.stmts[e.a].source; break;
            case TimelineEvent::Shunt: cap = R.exprs[e.a].shunt[e.b].note; break;
            case TimelineEvent::Gen: cap = R.exprs[e.a].gen[e.b].note; break;
            case TimelineEvent::Symbol: cap = "insert '" + R.symbols[e.a].name + "' into the symbol table"; break;
            case TimelineEvent::Emit: cap = e.note; break;
            case TimelineEvent::Done: cap = "Compilation finished - " + std::to_string(R.code.size()) + " lines of assembly"; break;
        }
        D::text(Face::Regular, 12, prog.x + bw + 8, prog.y - 1, D::ellipsize(Face::Regular, 12, cap, prog.w - bw - 10),
                theme::textDim);
    } else {
        D::textIn(Face::Regular, 13, prog, sim.asmMode ? "Assembly mode - the program is assembled directly"
                                                       : "Press Compile to start the animation",
                  theme::textMute);
    }

    // hash method selector for the symbol table
    Rect hs(bb.r() - 320, bb.y + 2, 320, 34);
    int hm = int(sim.hashMethod);
    int nh = ui.segmented("hashm", hs, {"Linear", "Quadratic", "Chaining"}, hm, theme::pink);
    if (ui.hovered(hs)) ui.tooltip("Collision strategy of the symbol & label tables (PL CO5)");
    if (nh != hm) {
        sim.hashMethod = ds::HashMethod(nh);
        sim.compile();
    }
}

// --------------------------------------------------------- English -> code
void CompileView::drawEnglish(const Rect& r, Sim& sim, UI& ui) {
    const core::EnglishResult& E = sim.english;
    Rect c = ui.card(r, "ENGLISH  \xE2\x86\x92  CODE  \xC2\xB7  TRANSLATOR", theme::violet,
                     E.ok && !E.sentences.empty() ? std::to_string(E.sentences.size()) + " sentences" : "");
    if (!E.ok || E.sentences.empty()) {
        ui.emptyState(c, E.ok ? "Nothing to translate" : "Sentence not understood",
                      E.ok ? "Type a sentence such as   add 12 and 3   and press Compile." : E.error);
        return;
    }
    Rect legend = c.cutBottom(20);
    c.cutBottom(6);
    const float chipH = 26, lineH = 21;
    // measure each sentence block
    std::vector<float> heights;
    for (const auto& s : E.sentences) {
        float x = 0, rows = 1;
        for (const auto& w : s.words) {
            float ww = D::textWidth(Face::Semibold, 13, w.text) + 16;
            if (x + ww > c.w - 4 && x > 0) { x = 0; ++rows; }
            x += ww + 5;
        }
        heights.push_back(22 + rows * (chipH + 5) + 8 + s.code.size() * lineH + 16);
    }
    int n = int(E.sentences.size());
    float total = n * kSentenceTime + 0.4f;
    bool done = transT_ >= total;
    int shown = done ? n : std::min(n, int(transT_ / kSentenceTime) + 1);
    float bottom = 0;
    for (int i = 0; i < shown; ++i) bottom += heights[i];
    float scroll = ui.anim("engScroll", std::max(0.f, bottom - c.h), 6);

    D::pushClip(Rect(c.x - 6, c.y - 4, c.w + 12, c.h + 8));
    float y = c.y - scroll;
    for (int i = 0; i < shown; ++i) {
        const auto& s = E.sentences[i];
        float t = done ? 99.f : transT_ - i * kSentenceTime;
        bool current = !done && i == shown - 1;
        Rect block(c.x - 4, y - 2, c.w + 8, heights[i] - 8);
        if (current) {
            D::roundRect(block, 10, theme::violet.alpha(0.07f));
            D::roundRectOutline(block, 10, theme::violet.alpha(0.35f));
        }
        // header: line number + matched rule
        D::text(Face::Bold, 10, c.x + 2, y + 2, "SENTENCE " + std::to_string(i + 1) + "  \xC2\xB7  line " +
                                                    std::to_string(s.line + 1), theme::textMute);
        if (t > 0.45f) {
            float rw = D::textWidth(Face::Bold, 10, s.rule) + 16;
            float ra = clampf((t - 0.45f) / 0.3f, 0, 1);
            Rect rr(c.r() - rw, y - 1, rw, 18);
            D::roundRect(rr, 9, theme::violet.alpha(0.25f * ra));
            D::roundRectOutline(rr, 9, theme::violet.alpha(0.7f * ra));
            D::textIn(Face::Bold, 10, rr, s.rule, theme::violet.lighten(0.4f).alpha(ra), Align::Center);
        }
        // the words, coloured by the role the translator gave them
        float x = c.x, wy = y + 22;
        for (std::size_t k = 0; k < s.words.size(); ++k) {
            const auto& w = s.words[k];
            float ww = D::textWidth(Face::Semibold, 13, w.text) + 16;
            if (x + ww > c.r() - 4 && x > c.x) { x = c.x; wy += chipH + 5; }
            float a = clampf((t - k * 0.06f) / 0.25f, 0, 1);
            Color col = roleColor(w.role);
            Rect wr(x, wy + (1 - easeOutCubic(a)) * 8, ww, chipH);
            if (w.role == core::WordRole::Filler || w.role == core::WordRole::Keyword) {
                D::roundRect(wr, 7, theme::bg1.alpha(a));
                D::textIn(Face::Semibold, 13, wr, w.text, col.alpha(a), Align::Center);
            } else {
                D::roundRectGrad(wr, 7, col.alpha(0.3f * a), col.alpha(0.16f * a));
                D::roundRectOutline(wr, 7, col.alpha(0.7f * a));
                D::textIn(Face::Semibold, 13, wr, w.text, col.lighten(0.35f).alpha(a), Align::Center);
            }
            x += ww + 5;
        }
        // arrow + generated statements
        float cy = wy + chipH + 8;
        float ca = clampf((t - 0.7f) / 0.3f, 0, 1);
        if (ca > 0) {
            D::icon(Icon::ChevronR, Rect(c.x - 2, cy + 1, 16, 16), theme::green.alpha(ca));
            for (std::size_t k = 0; k < s.code.size(); ++k) {
                float la = clampf((t - 0.7f - k * 0.12f) / 0.3f, 0, 1);
                if (la <= 0) break;
                drawStmtLine(c.x + 18 + (1 - la) * 10, cy + k * lineH, s.code[k], la, 13);
            }
        }
        y += heights[i];
        if (i < shown - 1) D::rect(Rect(c.x, y - 9, c.w, 1), theme::border.alpha(0.7f));
    }
    D::popClip();

    float lx = legend.x;
    const std::pair<const char*, core::WordRole> leg[] = {{"verb", core::WordRole::Verb}, {"number", core::WordRole::Number},
                                                          {"variable", core::WordRole::Variable},
                                                          {"operator", core::WordRole::Operator},
                                                          {"destination", core::WordRole::Target}};
    for (const auto& l : leg) {
        D::circle(Vec2(lx + 5, legend.y + 10), 4, roleColor(l.second));
        lx += D::text(Face::Regular, 11, lx + 13, legend.y + 3, l.first, theme::textMute) + 24;
    }
}

// ------------------------------------------------------------------ tokens
void CompileView::drawTokens(const Rect& r, Sim& sim, UI& ui, int ev) {
    const core::CompileResult& R = sim.compiled;
    Rect c = ui.card(r, "LEXER  \xC2\xB7  TOKEN STREAM", theme::cyan,
                     R.lex.empty() ? "" : std::to_string(R.lex.size()) + " tokens");
    if (R.lex.empty()) {
        ui.emptyState(c, "No tokens yet", "The lexer turns characters into tokens: numbers, names, operators and keywords.");
        return;
    }
    if (tokenBorn_.size() != R.lex.size()) tokenBorn_.assign(R.lex.size(), -1.f);
    int revealed = 0, latest = -1;
    for (int i = 0; i <= ev && i < int(R.timeline.size()); ++i)
        if (R.timeline[i].kind == TimelineEvent::Lex) { revealed = R.timeline[i].a + 1; latest = R.timeline[i].a; }
    Rect list = c;
    Rect kw = list.cutBottom(86);
    D::pushClip(list);
    float x = list.x, y = list.y;
    for (int i = 0; i < revealed; ++i) {
        const auto& t = R.lex[i].tok;
        if (tokenBorn_[i] < 0) tokenBorn_[i] = ui.time;
        float age = ui.time - tokenBorn_[i];
        float k = easeOutBack(age / 0.35f);
        float w = pillWidth(t.text);
        if (x + w > list.r()) { x = list.x; y += 38; }
        Rect pr(x, y + (1 - k) * 14, w, 30);
        bool cur = i == latest && R.timeline[ev].kind == TimelineEvent::Lex;
        drawPill(pr, t.text, tokColor(t.type), clampf(age / 0.2f, 0, 1), cur);
        x += w + 7;
    }
    D::popClip();
    // legend + keyword binary search
    D::rect(Rect(kw.x, kw.y + 2, kw.w, 1), theme::border);
    const auto& kws = core::Compiler::keywordTable();
    D::text(Face::Bold, 11, kw.x, kw.y + 12, "KEYWORD LOOKUP  \xC2\xB7  binary search", theme::textMute);
    int probeTok = latest;
    if (probeTok >= 0 && R.lex[probeTok].keywordProbes.empty()) {
        for (int i = latest; i >= 0; --i) if (!R.lex[i].keywordProbes.empty()) { probeTok = i; break; }
    }
    float bx = kw.x;
    float cellW = std::min(76.f, kw.w / float(kws.size()));
    for (int i = 0; i < int(kws.size()); ++i) {
        Rect cell(bx + i * cellW, kw.y + 34, cellW - 5, 24);
        int order = -1;
        if (probeTok >= 0)
            for (int k = 0; k < int(R.lex[probeTok].keywordProbes.size()); ++k)
                if (R.lex[probeTok].keywordProbes[k] == i) order = k;
        bool hit = probeTok >= 0 && R.lex[probeTok].keywordHit && order == int(R.lex[probeTok].keywordProbes.size()) - 1;
        Color col = hit ? theme::green : (order >= 0 ? theme::violet : theme::textMute);
        D::roundRect(cell, 6, col.alpha(order >= 0 ? 0.2f : 0.06f));
        D::roundRectOutline(cell, 6, col.alpha(0.5f));
        D::textIn(Face::MonoBold, cellW < 60 ? 11 : 12, cell, kws[i], order >= 0 ? theme::text : theme::textMute, Align::Center);
        if (order >= 0) D::text(Face::Bold, 9, cell.r() - 3, cell.y - 9, std::to_string(order + 1), col, Align::Right);
    }
    if (probeTok >= 0) {
        const auto& lx = R.lex[probeTok];
        std::string res = "'" + lx.tok.text + "'  " + (lx.keywordHit ? "is a keyword" : "is not a keyword \xE2\x86\x92 identifier");
        float lw0 = D::textWidth(Face::Bold, 11, "KEYWORD LOOKUP  \xC2\xB7  binary search") + 16;
        D::text(Face::Regular, 12, kw.x + lw0, kw.y + 12, D::ellipsize(Face::Regular, 12, res, kw.w - lw0),
                lx.keywordHit ? theme::green : theme::textDim);
    }
    // legend
    float lx = kw.x;
    kw.y += 22;
    const std::pair<const char*, TokType> legend[] = {{"number", TokType::Number}, {"identifier", TokType::Ident},
                                                      {"keyword", TokType::Keyword}, {"operator", TokType::Op},
                                                      {"relop", TokType::RelOp}, {"assign", TokType::Assign}};
    for (const auto& l : legend) {
        D::circle(Vec2(lx + 5, kw.y + 49), 4, tokColor(l.second));
        lx += D::text(Face::Regular, 11, lx + 13, kw.y + 42, l.first, theme::textMute) + 26;
    }
}

// ------------------------------------------------------------------ parser
void CompileView::drawParser(const Rect& r, Sim& sim, UI& ui, int ev) {
    const core::CompileResult& R = sim.compiled;
    int e = -1, st = -1;
    for (int i = 0; i <= ev && i < int(R.timeline.size()); ++i)
        if (R.timeline[i].kind == TimelineEvent::Shunt) { e = R.timeline[i].a; st = R.timeline[i].b; }
    Rect c = ui.card(r, "PARSER  \xC2\xB7  SHUNTING-YARD", theme::violet, "infix \xE2\x86\x92 postfix");
    if (e < 0) {
        ui.emptyState(c, "Waiting for an expression",
                      "Dijkstra's shunting-yard algorithm uses a STACK for operators and a QUEUE for the output.");
        return;
    }
    const core::ExprTrace& ex = R.exprs[e];
    const core::ShuntStep& s = ex.shunt[st];
    bool hot = R.timeline[ev].kind == TimelineEvent::Shunt;
    float t = hot ? easeOutCubic(evTime_ / 0.4f) : 1.f;

    D::text(Face::MonoBold, 15, c.x, c.y - 2, D::ellipsize(Face::MonoBold, 15, ex.context, c.w), theme::text);
    // infix tokens, highlight the one being consumed
    int consumed = 0;
    for (int i = 0; i <= st; ++i) {
        auto a = ex.shunt[i].action;
        if (a == core::ShuntStep::Operand || a == core::ShuntStep::PushOp || a == core::ShuntStep::PushParen ||
            a == core::ShuntStep::DropParen)
            ++consumed;
    }
    float x = c.x;
    D::text(Face::Bold, 11, c.x, c.y + 24, "INFIX INPUT", theme::textMute);
    for (int i = 0; i < int(ex.infix.size()); ++i) {
        float w = pillWidth(ex.infix[i], 13);
        bool cur = i == consumed - 1 && s.action != core::ShuntStep::PopOp && s.action != core::ShuntStep::Done;
        float al = i < consumed ? 0.35f : 1.f;
        if (cur) al = 1.f;
        drawPill(Rect(x, c.y + 42, w, 28), ex.infix[i], cur ? theme::violet : theme::textDim, al, cur, 13);
        x += w + 5;
    }

    // operator stack (vertical) and output queue (horizontal)
    Rect stackR(c.x, c.y + 86, 96, c.h - 86 - 50);
    D::roundRect(stackR, 10, theme::bg1);
    D::roundRectOutline(stackR, 10, theme::border);
    D::text(Face::Bold, 10, stackR.center().x, stackR.b() + 6, "OPERATOR STACK", theme::textMute, Align::Center);
    for (int i = 0; i < int(s.stack.size()); ++i) {
        bool top = i == int(s.stack.size()) - 1;
        float yoff = (top && hot && s.action == core::ShuntStep::PushOp) ? (1 - t) * -26 : 0;
        Rect pr(stackR.x + 10, stackR.b() - 10 - (i + 1) * 32 + yoff, stackR.w - 20, 28);
        std::string label = s.stack[i] == "neg" ? "u-" : s.stack[i];
        drawPill(pr, label, s.stack[i] == "(" ? theme::textDim : theme::pink, top && hot ? t : 1.f, top && hot, 13);
    }
    Rect qR(stackR.r() + 14, c.y + 86, c.r() - stackR.r() - 14, 46);
    D::roundRect(qR, 10, theme::bg1);
    D::roundRectOutline(qR, 10, theme::border);
    D::text(Face::Bold, 10, qR.x + 4, qR.b() + 6, "OUTPUT QUEUE  (front \xE2\x86\x92 back)", theme::textMute);
    x = qR.x + 9;
    for (int i = 0; i < int(s.output.size()); ++i) {
        std::string label = s.output[i] == "neg" ? "u-" : s.output[i];
        float w = pillWidth(label, 13);
        bool last = i == int(s.output.size()) - 1 && hot &&
                    (s.action == core::ShuntStep::Operand || s.action == core::ShuntStep::PopOp);
        float xo = last ? (1 - t) * 30 : 0;
        Color col = std::isdigit(static_cast<unsigned char>(label[0])) ? theme::amber
                    : std::isalpha(static_cast<unsigned char>(label[0])) ? theme::ident : theme::pink;
        if (label == "u-") col = theme::pink;
        drawPill(Rect(x + xo, qR.y + 9, w, 28), label, col, last ? t : 1.f, last, 13);
        x += w + 5;
    }
    // explanation
    Rect noteR(qR.x, qR.b() + 30, qR.w, c.b() - qR.b() - 30);
    D::roundRect(noteR, 10, theme::violet.alpha(0.07f));
    D::roundRectOutline(noteR, 10, theme::violet.alpha(0.25f));
    D::paragraph(Face::Regular, 13, noteR.inset(12, 10), s.note, theme::text.alpha(0.9f));
}

// ---------------------------------------------------------------- assembly
void CompileView::drawAssembly(const Rect& r, Sim& sim, UI& ui, int ev) {
    const core::CompileResult& R = sim.compiled;
    std::string right = sim.program ? std::to_string(sim.program->code.size()) + " instructions" : "";
    Rect c = ui.card(r, "GENERATED ASSEMBLY", theme::green, right);
    std::vector<std::string> lines;
    std::vector<float> glow;
    std::vector<int> progLine;   // program line index for each displayed line
    int execLine = sim.currentLine();
    if (sim.asmMode) {
        if (!sim.program) { ui.emptyState(c, "No program", "Assembly errors are shown in the editor."); return; }
        for (int i = 0; i < int(sim.program->lines.size()); ++i) {
            lines.push_back(sim.program->lines[i]);
            glow.push_back(0);
            progLine.push_back(i);
        }
    } else {
        if (!R.ok) { ui.emptyState(c, "Nothing generated", "Fix the compile error first."); return; }
        std::vector<char> shown(R.code.size(), 0);
        std::vector<float> hotness(R.code.size(), 0);
        int symCount = 0;
        for (int i = 0; i <= ev && i < int(R.timeline.size()); ++i) {
            const auto& e = R.timeline[i];
            for (int l : e.lines) {
                shown[l] = 1;
                if (i == ev) hotness[l] = 1 - clampf(evTime_ / 1.2f, 0, 1);
            }
            if (e.kind == TimelineEvent::Symbol) ++symCount;
        }
        bool done = ev >= int(R.timeline.size()) - 1;
        lines.push_back("section .data");
        glow.push_back(0);
        progLine.push_back(0);
        for (int i = 0; i < int(R.symbols.size()); ++i) {
            if (i >= symCount && !done) break;
            std::string nm = R.symbols[i].name;
            lines.push_back("    " + nm + std::string(nm.size() < 10 ? 10 - nm.size() : 1, ' ') + "dq 0");
            bool fresh = i == symCount - 1 && R.timeline[ev].kind == TimelineEvent::Symbol;
            glow.push_back(fresh ? 1 - clampf(evTime_ / 1.5f, 0, 1) : 0);
            progLine.push_back(1 + i);
        }
        lines.push_back("");
        glow.push_back(0); progLine.push_back(-1);
        lines.push_back("section .text");
        glow.push_back(0); progLine.push_back(-1);
        lines.push_back("_start:");
        glow.push_back(0); progLine.push_back(R.codeOffset - 1);
        for (int i = 0; i < int(R.code.size()); ++i) {
            if (!shown[i] && !done) continue;
            lines.push_back(R.code[i]);
            glow.push_back(hotness[i]);
            progLine.push_back(R.codeOffset + i);
        }
    }
    Font& mono = Fonts::get(Face::Mono, 14);
    float lh = std::round(mono.height() * 1.5f);
    float contentH = lines.size() * lh;
    // keep the newest / executing line in view
    float target = 0;
    int focus = -1;
    for (int i = 0; i < int(lines.size()); ++i) if (glow[i] > 0.01f) focus = i;
    for (int i = 0; i < int(lines.size()); ++i) if (progLine[i] == execLine && sim.hasCurrent) focus = i;
    if (focus < 0 && !sim.asmMode) focus = int(lines.size()) - 1;
    if (focus >= 0) target = clampf(focus * lh - c.h * 0.5f, 0, std::max(0.f, contentH - c.h));
    float scroll = ui.anim("asmScroll", target, 8);
    D::pushClip(c.inset(-4, 0));
    for (int i = 0; i < int(lines.size()); ++i) {
        float y = c.y + i * lh - scroll;
        if (y + lh < c.y - 4 || y > c.b() + 4) continue;
        Rect lr(c.x - 4, y, c.w + 8, lh);
        bool exec = sim.hasCurrent && progLine[i] == execLine && execLine >= 0;
        if (exec) {
            D::roundRect(lr, 6, theme::amber.alpha(0.16f));
            D::rect(Rect(lr.x, lr.y + 3, 3, lr.h - 6), theme::amber);
        }
        if (glow[i] > 0.01f) {
            D::roundRect(lr, 6, theme::green.alpha(0.22f * glow[i]));
            D::glow(lr.inset(4, 2), 6, 6 * glow[i], theme::green.alpha(glow[i]));
        }
        if (progLine[i] >= 0) D::text(Face::Mono, 11, c.x + 22, y + (lh - 14) / 2 + 1, std::to_string(progLine[i] + 1),
                                     theme::textMute.alpha(0.7f), Align::Right);
        float slide = glow[i] > 0.01f ? (glow[i] * glow[i]) * 12 : 0;
        drawAsmLine(c.x + 32 + slide, y + (lh - mono.height()) / 2, lines[i], 1.f, 14);
    }
    D::popClip();
}

// ----------------------------------------------------------------- symbols
void CompileView::drawSymbols(const Rect& r, Sim& sim, UI& ui, int ev) {
    const core::CompileResult& R = sim.compiled;
    ds::HashTable* table = nullptr;
    if (sim.asmMode && sim.program) table = sim.program->labelTable.get();
    else if (R.symtab) table = R.symtab.get();
    std::string title = std::string(sim.asmMode ? "LABEL TABLE" : "SYMBOL TABLE") + "  \xC2\xB7  " +
                        (table ? table->name() : std::string("HASHING"));
    Rect c = ui.card(r, title, theme::pink, "");
    if (!table) { ui.emptyState(c, "Empty", "Variables are hashed into this table as the compiler meets them."); return; }

    // which symbol is being inserted right now?
    int symIdx = -1, symCount = int(R.symbols.size());
    bool hot = false;
    if (!sim.asmMode) {
        symCount = 0;
        for (int i = 0; i <= ev && i < int(R.timeline.size()); ++i)
            if (R.timeline[i].kind == TimelineEvent::Symbol) { symIdx = R.timeline[i].a; symCount = symIdx + 1; }
        hot = R.timeline.size() && R.timeline[ev].kind == TimelineEvent::Symbol;
        if (ev >= int(R.timeline.size()) - 1) symCount = int(R.symbols.size());
    }
    auto buckets = table->buckets();
    int cap = int(buckets.size());
    // only show keys inserted so far (by insertion order)
    std::vector<std::string> visible;
    if (sim.asmMode) for (auto& l : sim.program->labels) visible.push_back(l.first);
    else for (int i = 0; i < symCount; ++i) visible.push_back(R.symbols[i].name);
    auto isVisible = [&](const std::string& k) { return std::find(visible.begin(), visible.end(), k) != visible.end(); };

    Rect info = c.cutTop(44);
    if (symIdx >= 0 && !sim.asmMode) {
        const auto& tr = R.symbols[symIdx].trace;
        char b[160];
        std::snprintf(b, sizeof b, "h(\"%s\") = %llu mod %d = %d", tr.key.c_str(), static_cast<unsigned long long>(tr.rawHash),
                      tr.capacity, tr.home);
        D::text(Face::MonoBold, 13, info.x, info.y, D::ellipsize(Face::MonoBold, 13, b, info.w), theme::pink.lighten(0.3f));
        std::string res = tr.collision ? "collision! probed " + std::to_string(tr.probes.size()) + " slot(s)"
                                       : "no collision";
        if (tr.rehashed) res += "  \xC2\xB7  table grew from " + std::to_string(tr.oldCapacity) + " (rehash)";
        D::text(Face::Regular, 12, info.x, info.y + 20, table->formula() + "   \xE2\x86\x92   " + res,
                tr.collision ? theme::orange : theme::textDim);
    } else {
        D::text(Face::Regular, 12, info.x, info.y + 4, table->formula(), theme::textDim);
    }

    float rowH = std::max(18.f, std::min(30.f, c.h / std::max(1, cap)));
    bool compact = rowH < 24;
    int cols = 1;
    if (cap * 22 > c.h) { cols = 2; rowH = std::max(18.f, std::min(28.f, c.h / ((cap + 1) / 2))); }
    int perCol = (cap + cols - 1) / cols;
    float colW = c.w / cols;
    const ds::ProbeTrace* tr = (symIdx >= 0 && !sim.asmMode) ? &R.symbols[symIdx].trace : nullptr;
    float pt = hot ? evTime_ / 1.2f : 1.f;   // probe progress
    for (int i = 0; i < cap; ++i) {
        int col = i / perCol, row = i % perCol;
        Rect br(c.x + col * colW, c.y + row * rowH, colW - 6, rowH - 3);
        D::roundRect(br, 6, theme::bg1);
        D::text(Face::MonoBold, compact ? 10 : 11, br.x + 8, br.y + (br.h - 12) / 2, std::to_string(i), theme::textMute);
        // probe highlight
        if (tr) {
            int n = int(tr->probes.size());
            for (int k = 0; k < n; ++k) {
                if (tr->probes[k] != i) continue;
                float at = float(k) / n;
                if (pt < at) continue;
                bool final = k == n - 1;
                Color pc = final ? theme::green : theme::red;
                float fresh = clampf(1 - (pt - at) * 2, 0, 1);
                D::roundRect(br, 6, pc.alpha(0.12f + 0.25f * fresh));
                D::roundRectOutline(br, 6, pc.alpha(0.5f + 0.5f * fresh));
            }
        }
        float x = br.x + 30;
        int pos = 0;
        for (const auto& kv : buckets[i]) {
            if (!isVisible(kv.first)) { ++pos; continue; }
            bool isNew = tr && kv.first == tr->key && hot;
            if (isNew && pt < 0.95f) { ++pos; continue; }
            std::string label = kv.first;
            float w = D::textWidth(Face::MonoBold, 12, label) + 16;
            if (pos > 0) {
                D::line(Vec2(x - 10, br.center().y), Vec2(x - 2, br.center().y), theme::textMute, 1.5f);
                D::arrowHead(Vec2(x - 1, br.center().y), Vec2(1, 0), 5, theme::textMute);
            }
            Rect kr(x, br.y + 2, w, br.h - 4);
            drawPill(kr, label, theme::pink, 1.f, isNew, 12);
            if (!compact && cols == 1) {
                char ad[20];
                std::snprintf(ad, sizeof ad, sim.asmMode ? "#%d" : "0x%X", kv.second);
                D::text(Face::Mono, 11, kr.r() + 6, br.y + (br.h - 12) / 2, ad, theme::textMute);
                x += w + 60;
            } else {
                x += w + 14;
            }
            ++pos;
        }
    }
    char st[96];
    std::snprintf(st, sizeof st, "load %.2f  \xC2\xB7  probes %d  \xC2\xB7  collisions %d", table->loadFactor(),
                  table->totalProbes(), table->collisions());
    // only when it fits next to the title
    if (D::textWidth(Face::Bold, 12, title) + D::textWidth(Face::Regular, 11, st) + 70 < r.w)
        D::text(Face::Regular, 11, c.r(), r.y + 13, st, theme::textMute, Align::Right);
    (void)ui;
}

} // namespace vas::ui
