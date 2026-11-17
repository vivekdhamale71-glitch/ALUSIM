// Shared helpers for the views: syntax highlighting and the register panel.
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>

#include "COA_2310221L/CO2_AssemblyISA/Assembler.h"
#include "CG_2310218L/CO3_ObjectAlgorithms/Raster.h"
#include "app/Views.h"

namespace vas::ui {

using namespace gfx;
namespace D = gfx::draw;

std::string hex64(std::uint64_t v, bool grouped) {
    char b[24];
    std::snprintf(b, sizeof b, "%016llX", static_cast<unsigned long long>(v));
    std::string s = b;
    if (!grouped) return "0x" + s;
    return s.substr(0, 4) + "_" + s.substr(4, 4) + "_" + s.substr(8, 4) + "_" + s.substr(12, 4);
}

static bool isWordChar(char c) { return std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '.'; }

Highlighter statementHighlighter() {
    return [](const std::string& ln) {
        std::vector<HighlightSpan> s;
        std::size_t i = 0;
        while (i < ln.size()) {
            char c = ln[i];
            if (c == '#' || (c == '/' && i + 1 < ln.size() && ln[i + 1] == '/')) {
                s.push_back({int(i), int(ln.size()), theme::textMute});
                break;
            }
            if (std::isdigit(static_cast<unsigned char>(c))) {
                std::size_t j = i;
                while (j < ln.size() && std::isalnum(static_cast<unsigned char>(ln[j]))) ++j;
                s.push_back({int(i), int(j), theme::amber});
                i = j;
            } else if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
                std::size_t j = i;
                while (j < ln.size() && isWordChar(ln[j])) ++j;
                std::string w = ln.substr(i, j - i);
                Color col = (w == "if" || w == "else" || w == "while") ? theme::violet : theme::ident;
                s.push_back({int(i), int(j), col});
                i = j;
            } else if (std::strchr("+-*/%&|^~<>!=", c)) {
                s.push_back({int(i), int(i + 1), theme::pink});
                ++i;
            } else if (std::strchr("(){};", c)) {
                s.push_back({int(i), int(i + 1), theme::textDim});
                ++i;
            } else {
                ++i;
            }
        }
        return s;
    };
}

Highlighter englishHighlighter() {
    return [](const std::string& ln) {
        static const char* verbs[] = {"add", "subtract", "multiply", "divide", "set", "let", "find", "compute",
                                      "calculate", "if", "then", "otherwise", "else", "repeat", "while", "increase",
                                      "decrease", "increment", "decrement", "double", "triple", "halve", "square",
                                      "swap", "store", "put", "save", "raise", "what", "evaluate", "make", "assign"};
        static const char* ops[] = {"plus", "minus", "times", "by", "divided", "multiplied", "over", "mod", "greater",
                                    "less", "than", "equals", "equal", "least", "most", "factorial", "sum", "largest",
                                    "smallest", "power", "remainder", "product", "average"};
        std::vector<HighlightSpan> s;
        std::size_t i = 0;
        while (i < ln.size()) {
            if (ln[i] == '#' || (ln[i] == '/' && i + 1 < ln.size() && ln[i + 1] == '/')) {
                s.push_back({int(i), int(ln.size()), theme::textMute});
                break;
            }
            if (!std::isalnum(static_cast<unsigned char>(ln[i]))) { ++i; continue; }
            std::size_t j = i;
            while (j < ln.size() && (std::isalnum(static_cast<unsigned char>(ln[j])) || ln[j] == '_')) ++j;
            std::string w = ln.substr(i, j - i);
            for (auto& c : w) c = char(std::tolower(static_cast<unsigned char>(c)));
            Color col = theme::text;
            if (std::isdigit(static_cast<unsigned char>(w[0]))) col = theme::amber;
            for (const char* v : verbs) if (w == v) col = theme::violet;
            for (const char* o : ops) if (w == o) col = theme::pink;
            s.push_back({int(i), int(j), col});
            i = j;
        }
        return s;
    };
}

Highlighter asmHighlighter() {
    return [](const std::string& ln) {
        std::vector<HighlightSpan> s;
        std::size_t i = 0;
        bool first = true;
        std::size_t sc = ln.find(';');
        std::size_t end = sc == std::string::npos ? ln.size() : sc;
        while (i < end) {
            char c = ln[i];
            if (std::isspace(static_cast<unsigned char>(c)) || c == ',') { ++i; continue; }
            if (c == '[' || c == ']') { s.push_back({int(i), int(i + 1), theme::orange}); ++i; continue; }
            if (c == '+' || c == '-') { s.push_back({int(i), int(i + 1), theme::pink}); ++i; continue; }
            std::size_t j = i;
            while (j < end && (isWordChar(ln[j]) || ln[j] == ':')) ++j;
            if (j == i) { ++i; continue; }
            std::string w = ln.substr(i, j - i);
            std::string lw;
            for (char ch : w) lw += char(std::tolower(static_cast<unsigned char>(ch)));
            Color col = theme::text;
            if (!w.empty() && w.back() == ':') col = theme::green;
            else if (lw == "section" || lw == "global" || lw == "dq" || lw == ".data" || lw == ".text" || lw == "qword")
                col = theme::textMute;
            else if (core::Assembler::registerIndex(lw) >= 0 || lw == "cl") col = theme::ident;
            else if (std::isdigit(static_cast<unsigned char>(w[0])) || (w[0] == '-' && w.size() > 1)) col = theme::amber;
            else if (first) col = theme::violet;
            else col = theme::text;
            s.push_back({int(i), int(j), col});
            if (!(w.back() == ':')) first = false;
            i = j;
        }
        if (sc != std::string::npos) s.push_back({int(sc), int(ln.size()), theme::textMute});
        return s;
    };
}

void drawAsmLine(float x, float y, const std::string& line, float alpha, int px) {
    static Highlighter hl = asmHighlighter();
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

// ------------------------------------------------------------ register panel
static int gSelectedReg = 0;

static void drawFlagLed(Vec2 c, const char* name, bool on, float flash, const Color& col) {
    // ring rasterised with the midpoint circle algorithm             [CG CO3]
    std::vector<IPoint> ring = midpointCircle(int(c.x), int(c.y), 11);
    std::vector<Vec2> pts;
    for (auto& p : ring) pts.push_back({float(p.x) + 0.5f, float(p.y) + 0.5f});
    if (on) {
        D::circle(c, 18 + flash * 6, col.alpha(0.12f + 0.15f * flash));
        D::circle(c, 8, col);
        D::circle(c - Vec2(2.5f, 2.5f), 2.5f, Color(1, 1, 1, 0.55f));
    } else {
        D::circle(c, 8, theme::bg1);
    }
    D::points(pts, 1.8f, on ? col.lighten(0.3f) : theme::borderHi);
    D::text(Face::Bold, 12, c.x, c.y + 16, name, on ? col.lighten(0.3f) : theme::textMute, Align::Center);
}

void drawRegisterPanel(const Rect& area, Sim& sim, UI& ui) {
    Rect a = area;
    float memH = std::max(118.f, a.h * 0.2f);
    Rect regCard = a.cutTop(a.h - memH - theme::gap);
    a.cutTop(theme::gap);
    Rect memCard = a;

    Rect c = ui.card(regCard, "REGISTERS", theme::cyan, "x86-64  \xC2\xB7  64-bit");

    // RIP
    std::uint64_t rip = core::CPU::addressOf(sim.cpu.pc());
    Rect ripR = c.cutTop(42);
    D::roundRectGrad(ripR, 10, theme::cyan.alpha(0.13f), theme::blue.alpha(0.06f));
    D::roundRectOutline(ripR, 10, theme::cyan.alpha(0.35f));
    D::text(Face::MonoBold, 13, ripR.x + 12, ripR.y + 6, "RIP", theme::cyan);
    D::text(Face::MonoBold, 15, ripR.x + 54, ripR.y + 5, hex64(rip), theme::text);
    std::string cur = sim.hasCurrent ? sim.current.text : "";
    D::text(Face::Mono, 11, ripR.x + 54, ripR.y + 24, D::ellipsize(Face::Mono, 11, cur.empty() ? "-" : cur, ripR.w - 64),
            theme::textDim);
    c.cutTop(8);

    // flags + sub-register view take the bottom
    Rect flags = c.cutBottom(58);
    Rect sub = c.cutBottom(74);
    c.cutBottom(6);

    // short screens: two columns of 8 with compact values
    bool compact = c.h / 16 < 19;
    float rowH = compact ? std::min(26.f, c.h / 8) : std::min(26.f, c.h / 16);
    Font& mono = Fonts::get(Face::Mono, 13);
    float cw = mono.width("0");
    if (compact) {
        float colW = (c.w - 8) / 2;
        for (int i = 0; i < core::REG_COUNT; ++i) {
            Rect row(c.x + (i / 8) * (colW + 8), c.y + (i % 8) * rowH, colW, rowH - 2);
            if (ui.clicked(row)) gSelectedReg = i;
            float fl = sim.regFlash[i];
            if (i == gSelectedReg) D::roundRect(row, 6, theme::panel3.alpha(0.9f));
            if (fl > 0.01f) {
                D::roundRect(row, 6, theme::amber.alpha(0.22f * fl));
                D::roundRectOutline(row, 6, theme::amber.alpha(0.6f * fl));
            }
            D::textIn(Face::MonoBold, 12, Rect(row.x + 6, row.y, 36, row.h), core::regName(i),
                      fl > 0.05f ? theme::amber : theme::ident);
            char b[24];
            std::snprintf(b, sizeof b, "0x%llX", static_cast<unsigned long long>(sim.shownRegs[i]));
            D::textIn(Face::Mono, 12, Rect(row.x, row.y, row.w - 6, row.h), b, theme::text, Align::Right);
        }
    }
    for (int i = 0; i < core::REG_COUNT && !compact; ++i) {
        Rect row(c.x, c.y + i * rowH, c.w, rowH - 2);
        bool hov = ui.hovered(row);
        if (ui.clicked(row)) gSelectedReg = i;
        float fl = sim.regFlash[i];
        if (i == gSelectedReg) D::roundRect(row, 7, theme::panel3.alpha(0.9f));
        else if (hov) D::roundRect(row, 7, theme::panel3.alpha(0.45f));
        if (fl > 0.01f) {
            D::roundRect(row, 7, theme::amber.alpha(0.22f * fl));
            D::roundRectOutline(row, 7, theme::amber.alpha(0.6f * fl));
        }
        bool written = sim.hasCurrent && std::find(sim.current.regsWritten.begin(), sim.current.regsWritten.end(), i) !=
                                             sim.current.regsWritten.end();
        Color nameC = fl > 0.05f ? theme::amber : (written ? theme::amber.lerp(theme::cyan, 0.4f) : theme::ident);
        D::textIn(Face::MonoBold, 13, Rect(row.x + 8, row.y, 40, row.h), core::regName(i), nameC);
        // value: leading zeros dimmed
        std::string hx = hex64(sim.shownRegs[i], true);
        std::size_t firstSig = hx.find_first_not_of("0_");
        if (firstSig == std::string::npos) firstSig = hx.size() - 1;
        float x = row.x + 50;
        float ty = row.y + (row.h - mono.height()) / 2;
        mono.draw(x, ty, hx.substr(0, firstSig), theme::textMute.alpha(0.7f));
        mono.draw(x + cw * firstSig, ty, hx.substr(firstSig), theme::text);
        std::int64_t sv = std::int64_t(sim.shownRegs[i]);
        std::string dec = (sv > -10000000 && sv < 100000000) ? std::to_string(sv) : "";
        D::textIn(Face::Mono, 11, Rect(row.x, row.y, row.w - 8, row.h), dec, theme::textMute, Align::Right);
    }

    // sub-register breakdown (8086 -> 80386 -> x86-64 lineage)            [COA CO1]
    D::roundRect(sub, 10, theme::bg1.alpha(0.8f));
    D::roundRectOutline(sub, 10, theme::border);
    std::uint64_t v = sim.shownRegs[gSelectedReg];
    static const char* e32[] = {"eax", "ebx", "ecx", "edx", "esi", "edi", "esp", "ebp"};
    static const char* x16[] = {"ax", "bx", "cx", "dx", "si", "di", "sp", "bp"};
    static const char* h8[] = {"ah", "bh", "ch", "dh"};
    static const char* l8[] = {"al", "bl", "cl", "dl"};
    int g = gSelectedReg;
    std::string n64 = core::regName(g);
    std::string n32 = g < 8 ? e32[g] : n64 + "d";
    std::string n16 = g < 8 ? x16[g] : n64 + "w";
    std::string nl = g < 4 ? l8[g] : n64 + "b";
    D::text(Face::Bold, 11, sub.x + 10, sub.y + 7, "SUB-REGISTERS OF " + n64 + "  (8086 \xE2\x86\x92 80386 \xE2\x86\x92 x86-64)",
            theme::textMute);
    Rect bar(sub.x + 10, sub.y + 26, sub.w - 20, 22);
    float bw = bar.w;
    auto seg = [&](float x0, float x1, const Color& col, const std::string& label) {
        Rect s(bar.x + bw * x0, bar.y, bw * (x1 - x0) - 2, bar.h);
        D::roundRect(s, 5, col.alpha(0.22f));
        D::roundRectOutline(s, 5, col.alpha(0.6f));
        D::textIn(Face::MonoBold, 11, s, label, col.lighten(0.3f), Align::Center);
    };
    seg(0, 0.5f, theme::blue, n64);
    seg(0.5f, 0.75f, theme::violet, n32);
    seg(0.75f, 0.875f, theme::pink, n16);
    if (g < 4) { seg(0.875f, 0.9375f, theme::amber, h8[g]); seg(0.9375f, 1.0f, theme::green, nl); }
    else seg(0.875f, 1.0f, theme::green, nl);
    char b[96];
    std::snprintf(b, sizeof b, "%s=%08X   %s=%04X   %s=%02X", n32.c_str(), unsigned(v & 0xFFFFFFFF), n16.c_str(),
                  unsigned(v & 0xFFFF), nl.c_str(), unsigned(v & 0xFF));
    D::text(Face::Mono, 11, sub.x + 10, sub.y + 53, b, theme::textDim);

    // flags
    const core::Flags& f = sim.shownFlags;
    float fx = flags.x + flags.w / 8;
    float step = flags.w / 4;
    drawFlagLed(Vec2(fx, flags.y + 18), "CF", f.CF, sim.flagFlash, theme::amber);
    drawFlagLed(Vec2(fx + step, flags.y + 18), "ZF", f.ZF, sim.flagFlash, theme::green);
    drawFlagLed(Vec2(fx + 2 * step, flags.y + 18), "SF", f.SF, sim.flagFlash, theme::pink);
    drawFlagLed(Vec2(fx + 3 * step, flags.y + 18), "OF", f.OF, sim.flagFlash, theme::red);

    // memory
    Rect m = ui.card(memCard, "DATA MEMORY", theme::pink,
                     sim.program ? std::to_string(sim.program->data.size()) + " variables" : "");
    if (!sim.program || sim.program->data.empty()) {
        D::textIn(Face::Regular, 12, m, "Variables appear here after compiling", theme::textMute, Align::Center);
        return;
    }
    D::pushClip(m);
    float y = m.y;
    for (const auto& d : sim.program->data) {
        Rect row(m.x, y, m.w, 24);
        bool flash = sim.memFlash > 0.01f && sim.lastMemWrite == d.addr;
        if (flash) D::roundRect(row, 6, theme::pink.alpha(0.25f * sim.memFlash));
        std::int64_t val = sim.cpu.loaded() ? sim.cpu.peek(d.addr) : d.init;
        D::textIn(Face::MonoBold, 13, Rect(row.x + 6, row.y, 90, row.h), d.name, theme::text);
        char ad[24];
        std::snprintf(ad, sizeof ad, "[0x%04llX]", static_cast<unsigned long long>(d.addr));
        D::textIn(Face::Mono, 11, Rect(row.x + 100, row.y, 90, row.h), ad, theme::textMute);
        D::textIn(Face::MonoBold, 13, Rect(row.x, row.y, row.w - 6, row.h), std::to_string(val),
                  flash ? theme::pink.lighten(0.3f) : theme::amber, Align::Right);
        y += 25;
    }
    D::popClip();
}

} // namespace vas::ui
