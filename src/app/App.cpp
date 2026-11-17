#include "app/App.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>

#include "CG_2310218L/CO1_Primitives/GL.h"
#include "app/Samples.h"

namespace vas::ui {

using namespace gfx;
namespace D = gfx::draw;

App& App::get() {
    static App a;
    return a;
}

static void cbDisplay() { App::get().display(); }
static void cbReshape(int w, int h) { App::get().reshape(w, h); }
static void cbKeyboard(unsigned char k, int x, int y) { App::get().keyboard(k, x, y); }
static void cbSpecial(int k, int x, int y) { App::get().special(k, x, y); }
static void cbMouse(int b, int s, int x, int y) { App::get().mouse(b, s, x, y); }
static void cbMotion(int x, int y) { App::get().motion(x, y); }
static void cbWheel(int, int dir, int, int) { App::get().wheel(dir); }
static void cbIdle() { glutPostRedisplay(); }

static bool fileExists(const std::string& p) {
    std::ifstream f(p);
    return bool(f);
}

void App::init(int argc, char** argv) {
    theme::apply(false);
    glutInit(&argc, argv);
    // locate the assets folder (next to the executable or the working directory)
    std::string exe = argv[0];
    std::string dir = exe.substr(0, exe.find_last_of("/\\") == std::string::npos ? 0 : exe.find_last_of("/\\"));
    for (const std::string& cand : {std::string("assets"), dir + "/assets", dir + "/../assets"}) {
        if (fileExists(cand + "/fonts/Lato-Regular.ttf")) { Fonts::setAssetDir(cand); break; }
    }

    int tab = 0, sample = -1, steps = 0, micro = 0;
    bool finish = false, compile = true, sizeGiven = false;
    std::string codeArg;
    int view3d = 0, subMode = -1;
    bool flat = false;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        auto next = [&]() { return i + 1 < argc ? std::string(argv[++i]) : std::string(); };
        if (a == "--shot") shotPath_ = next();
        else if (a == "--frames") shotFrames_ = std::atoi(next().c_str());
        else if (a == "--tab") tab = std::atoi(next().c_str());
        else if (a == "--sample") sample = std::atoi(next().c_str());
        else if (a == "--steps") steps = std::atoi(next().c_str());
        else if (a == "--finish") finish = true;
        else if (a == "--micro") micro = 1;
        else if (a == "--size") { w_ = std::atoi(next().c_str()); h_ = std::atoi(next().c_str()); sizeGiven = true; }
        else if (a == "--nocompile") compile = false;
        else if (a == "--chip") view3d = 1;
        else if (a == "--light") theme::apply(true);
        else if (a == "--sub") subMode = std::atoi(next().c_str());
        else if (a == "--flat") flat = true;
        else if (a == "--code") {
            std::string code = next(), out;
            for (std::size_t k = 0; k < code.size(); ++k) {
                if (code[k] == '\\' && k + 1 < code.size() && code[k + 1] == 'n') { out += '\n'; ++k; }
                else out += code[k];
            }
            codeArg = out;
        }
    }
    if (!shotPath_.empty() && shotFrames_ < 0) shotFrames_ = 90;
    fixedStep_ = !shotPath_.empty();

    glutSetOption(GLUT_MULTISAMPLE, 8);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH | GLUT_MULTISAMPLE);
    if (!sizeGiven) {
        int sw = glutGet(GLUT_SCREEN_WIDTH), sh = glutGet(GLUT_SCREEN_HEIGHT);
        if (sw > 0 && sh > 0) {
            w_ = std::max(1280, std::min(1760, int(sw * 0.94f)));
            h_ = std::max(760, std::min(1000, int(sh * 0.9f)));
        }
    }
    glutInitWindowSize(w_, h_);
    glutCreateWindow("Virtual ALU Simulator  -  Statement -> x86-64 -> Datapath");
    glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
    glEnable(GL_MULTISAMPLE);

    glutDisplayFunc(cbDisplay);
    glutReshapeFunc(cbReshape);
    glutKeyboardFunc(cbKeyboard);
    glutSpecialFunc(cbSpecial);
    glutMouseFunc(cbMouse);
    glutMotionFunc(cbMotion);
    glutPassiveMotionFunc(cbMotion);
    glutMouseWheelFunc(cbWheel);
    glutIdleFunc(cbIdle);

    views_.push_back(std::make_unique<CompileView>());
    views_.push_back(std::make_unique<DatapathView>());
    views_.push_back(std::make_unique<PipelineView>());
    views_.push_back(std::make_unique<ArithmeticView>());
    views_.push_back(std::make_unique<DataLabView>());
    views_.push_back(std::make_unique<ChipView>());
    tab_ = std::max(0, std::min(int(views_.size()) - 1, tab));
    static_cast<ChipView*>(views_[5].get())->configure(view3d, flat);
    if (subMode >= 0) views_[tab_]->setSub(subMode);

    if (sample >= 0) sim_.loadSample(sample);
    if (!codeArg.empty()) {
        sim_.source = codeArg;
        sim_.setInputMode(codeArg.find("section") != std::string::npos ? MODE_ASM
                          : (codeArg.find(';') != std::string::npos || codeArg.find('{') != std::string::npos) ? MODE_CPP
                                                                                                             : MODE_ENGLISH);
    }
    sim_.setMicro(micro != 0);
    if (compile) sim_.compile();
    if (finish) sim_.finish();
    for (int i = 0; i < steps; ++i) sim_.step();
    lastMs_ = glutGet(GLUT_ELAPSED_TIME);
}

void App::run() { glutMainLoop(); }

void App::reshape(int w, int h) {
    w_ = std::max(w, 1);
    h_ = std::max(h, 1);
}

// ------------------------------------------------------------------ input
void App::keyboard(unsigned char key, int, int) {
    UI& ui = UI::get();
    int mods = glutGetModifiers();
    ui.in.ctrl = mods & GLUT_ACTIVE_CTRL;
    ui.in.shift = mods & GLUT_ACTIVE_SHIFT;
    if (ui.in.ctrl) {
        if (key == 10 || key == 13) { sim_.compile(); return; }
        if (key == 19) {   // Ctrl+S
            bool ok = sim_.saveSource("my_program.txt");
            sim_.toast(ok ? "Saved" : "Save failed", ok ? "Source written to my_program.txt" : "Could not write file", !ok);
            return;
        }
        if (key == 15) {   // Ctrl+O
            bool ok = sim_.loadSource("my_program.txt");
            sim_.toast(ok ? "Loaded" : "Load failed", ok ? "my_program.txt loaded into the editor" : "my_program.txt not found", !ok);
            return;
        }
        if (key == 5) {    // Ctrl+E
            bool ok = sim_.exportTrace("trace_log.txt");
            sim_.toast(ok ? "Trace exported" : "Export failed", ok ? "Execution trace written to trace_log.txt" : "Could not write file", !ok);
            return;
        }
        return;
    }
    if (key == 10) { sim_.compile(); return; }
    ui.in.keys.push_back(key == 127 ? 127 : key);
}

void App::special(int key, int, int) {
    UI& ui = UI::get();
    int k = -1;
    switch (key) {
        case GLUT_KEY_LEFT: k = K_LEFT; break;
        case GLUT_KEY_RIGHT: k = K_RIGHT; break;
        case GLUT_KEY_UP: k = K_UP; break;
        case GLUT_KEY_DOWN: k = K_DOWN; break;
        case GLUT_KEY_HOME: k = K_HOME; break;
        case GLUT_KEY_END: k = K_END; break;
        case GLUT_KEY_PAGE_UP: k = K_PGUP; break;
        case GLUT_KEY_PAGE_DOWN: k = K_PGDN; break;
        case GLUT_KEY_DELETE: k = K_DELETE; break;
        default:
            if (key >= GLUT_KEY_F1 && key <= GLUT_KEY_F12) k = K_F1 + (key - GLUT_KEY_F1);
    }
    if (k >= 0) ui.in.keys.push_back(k);
}

void App::mouse(int button, int state, int x, int y) {
    UI& ui = UI::get();
    ui.in.mouse = Vec2(float(x), float(y));
    if (button == GLUT_LEFT_BUTTON) {
        if (state == GLUT_DOWN) { ui.in.down = true; ui.in.pressed = true; }
        else { ui.in.down = false; ui.in.released = true; }
    } else if (button == GLUT_RIGHT_BUTTON) {
        ui.in.rdown = state == GLUT_DOWN;
    } else if (button == 3 && state == GLUT_DOWN) {
        ui.in.wheel += 1;
    } else if (button == 4 && state == GLUT_DOWN) {
        ui.in.wheel -= 1;
    }
}

void App::motion(int x, int y) { UI::get().in.mouse = Vec2(float(x), float(y)); }
void App::wheel(int dir) { UI::get().in.wheel += float(dir); }

void App::handleShortcuts() {
    UI& ui = UI::get();
    for (int k : ui.in.keys) {
        if (k >= K_F1 && k < K_F1 + int(views_.size())) tab_ = k - K_F1;
        else if (k == K_F11) {
            fullscreen_ = !fullscreen_;
            if (fullscreen_) glutFullScreen();
            else { glutReshapeWindow(1600, 920); glutPositionWindow(60, 40); }
        }
        if (ui.keyboardTaken) continue;
        if (k == ' ' || k == K_F5) sim_.toggleRun();
        else if (k == K_F10 || k == K_RIGHT) sim_.step();
        else if (k == K_F9 || k == K_LEFT) sim_.stepBack();
    }
}

// ------------------------------------------------------------------ frame
void App::display() {
    int now = glutGet(GLUT_ELAPSED_TIME);
    float dt = fixedStep_ ? 1.0f / 60 : std::min(0.1f, (now - lastMs_) / 1000.0f);
    lastMs_ = now;

    UI& ui = UI::get();
    ui.beginFrame(dt);
    sim_.update(dt);

    glClearColor(theme::bg0.r, theme::bg0.g, theme::bg0.b, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    D::beginFrame(w_, h_);
    drawBackground();

    Rect full(0, 0, float(w_), float(h_));
    Rect top = full.cutTop(66);
    Rect status = full.cutBottom(32);
    Rect body = full.inset(theme::gap, theme::gap);
    Rect side = body.cutRight(344);
    body.w -= theme::gap;

    views_[tab_]->draw(body, sim_, ui);
    D::beginFrame(w_, h_);   // views may change GL state (3D)
    Rect ctrl = side.cutTop(166);
    side.cutTop(theme::gap);
    drawControlCard(ctrl);
    drawRegisterPanel(side, sim_, ui);
    drawTopBar(top);
    drawStatusBar(status);
    drawToast();
    handleShortcuts();
    ui.endFrame();

    ++frame_;
    if (!shotPath_.empty() && frame_ >= shotFrames_) {
        saveScreenshot();
        std::exit(0);
    }
    glutSwapBuffers();
}

void App::drawBackground() {
    D::rectGrad(Rect(0, 0, float(w_), float(h_)), theme::bg1, theme::bg0);
    // soft colour blobs
    auto blob = [](Vec2 c, float r, const Color& col) {
        glBegin(GL_TRIANGLE_FAN);
        glColor4f(col.r, col.g, col.b, col.a);
        glVertex2f(c.x, c.y);
        glColor4f(col.r, col.g, col.b, 0);
        for (int i = 0; i <= 48; ++i) {
            float a = 6.2831853f * i / 48;
            glVertex2f(c.x + std::cos(a) * r, c.y + std::sin(a) * r * 0.75f);
        }
        glEnd();
    };
    float t = UI::get().time;
    blob(Vec2(w_ * 0.18f + std::sin(t * 0.13f) * 40, h_ * 0.15f), w_ * 0.45f, theme::cyan.alpha(0.07f));
    blob(Vec2(w_ * 0.85f, h_ * 0.9f + std::cos(t * 0.11f) * 30), w_ * 0.5f, theme::violet.alpha(0.07f));
    blob(Vec2(w_ * 0.55f, h_ * 0.45f), w_ * 0.35f, theme::blue.alpha(0.03f));
    // faint dot grid
    std::vector<Vec2> dots;
    for (float y = 80; y < h_; y += 28)
        for (float x = 14; x < w_; x += 28) dots.push_back({x, y});
    D::points(dots, 1.6f, theme::light ? Color(0.1f, 0.2f, 0.4f, 0.07f) : Color(1, 1, 1, 0.035f));
}

void App::drawTopBar(const Rect& r) {
    UI& ui = UI::get();
    D::rectGrad(r, theme::panel.alpha(0.96f), theme::overlay.alpha(0.96f));
    D::rect(Rect(r.x, r.b() - 1, r.w, 1), theme::border);
    D::rectGradH(Rect(r.x, r.b() - 1, r.w * 0.5f, 1), theme::cyan.alpha(0.0f), theme::cyan.alpha(0.5f));
    D::rectGradH(Rect(r.x + r.w * 0.5f, r.b() - 1, r.w * 0.5f, 1), theme::violet.alpha(0.5f), theme::violet.alpha(0.0f));

    // logo
    Rect logo(r.x + 18, r.y + 13, 40, 40);
    D::glow(logo, 11, 10, theme::cyan.alpha(0.5f));
    D::roundRectGrad(logo, 11, Color::hex(0x38BDF8), Color::hex(0x7C3AED));
    D::icon(Icon::Chip, logo.inset(8), theme::onAccent);
    D::text(Face::Bold, 19, logo.r() + 14, r.y + 12, "Virtual ALU Simulator", theme::text);
    D::text(Face::Semibold, 12, logo.r() + 15, r.y + 37, "statement  \xE2\x86\x92  x86-64  \xE2\x86\x92  datapath  \xE2\x86\x92  pipeline",
            theme::textMute);

    // tabs: share the space between the title and the status chip
    float tx = logo.r() + 240;
    float rightEdge = r.r() - 18 - 270 - 60;
    float tabW = std::min(124.f, (rightEdge - tx) / float(views_.size()) - 4), tabH = 38;
    bool compactTabs = tabW < 104;
    float selX = ui.anim("tabsel", tx + tab_ * (tabW + 4), 18);
    Rect sel(selX, r.y + 14, tabW, tabH);
    D::glow(sel, 11, 8, theme::cyan.alpha(0.35f));
    D::roundRectGrad(sel, 11, theme::cyan.alpha(0.26f), theme::blue.alpha(0.16f));
    D::roundRectOutline(sel, 11, theme::cyan.alpha(0.55f));
    for (int i = 0; i < int(views_.size()); ++i) {
        Rect t(tx + i * (tabW + 4), r.y + 14, tabW, tabH);
        bool hov = ui.hovered(t);
        float h = ui.anim("tabh" + std::to_string(i), hov ? 1.f : 0.f);
        if (i != tab_ && h > 0.01f) D::roundRect(t, 11, theme::panel3.alpha(0.7f * h));
        Color c = i == tab_ ? theme::text : theme::textDim.lerp(theme::text, h);
        std::string label = views_[i]->name();
        int fpx = compactTabs ? 12 : 13;
        float lw = D::textWidth(Face::Semibold, fpx, label);
        bool showIcon = !compactTabs || lw + 24 < t.w - 8;
        float x = t.x + (t.w - lw - (showIcon ? 24 : 0)) / 2;
        if (showIcon) D::icon(views_[i]->icon(), Rect(x, t.y + 11, 16, 16), i == tab_ ? theme::cyan : c);
        D::text(i == tab_ ? Face::Bold : Face::Semibold, fpx, x + (showIcon ? 24 : 0), t.y + 11, label, c);
        if (ui.clicked(t)) tab_ = i;
        if (hov) ui.tooltip(label + "  (F" + std::to_string(i + 1) + ")");
    }

    // right: live status chip
    Color dot = sim_.running ? theme::green : (sim_.loaded() ? (sim_.cpu.halted() ? theme::amber : theme::cyan) : theme::textMute);
    std::string state = sim_.running ? "RUNNING" : (!sim_.loaded() ? "NO PROGRAM" : (sim_.cpu.halted() ? "HALTED" : "READY"));
    char info[96];
    std::snprintf(info, sizeof info, "cycle %ld  \xC2\xB7  %ld instr", core::Clock::instance().cycle(), sim_.cpu.steps());
    float cw = D::textWidth(Face::Bold, 12, state) + D::textWidth(Face::MonoBold, 12, info) + 64;
    Rect chip(r.r() - 18 - cw, r.y + 17, cw, 32);
    // dark / light theme switch
    Rect themeBtn(chip.x - 48, r.y + 15, 36, 36);
    if (ui.iconButton("theme", themeBtn, theme::light ? Icon::Moon : Icon::Sun,
                      theme::light ? "Switch to dark mode" : "Switch to light mode", Style::Secondary))
        theme::apply(!theme::light);
    D::roundRect(chip, 16, theme::bg1.alpha(0.9f));
    D::roundRectOutline(chip, 16, dot.alpha(0.45f));
    float pulse = sim_.running ? 0.5f + 0.5f * std::sin(ui.time * 6) : 0.3f;
    D::circle(Vec2(chip.x + 18, chip.center().y), 9, dot.alpha(0.18f * pulse + 0.05f));
    D::circle(Vec2(chip.x + 18, chip.center().y), 4.5f, dot);
    float tx2 = chip.x + 32;
    tx2 += D::textIn(Face::Bold, 12, Rect(tx2, chip.y, 100, chip.h), state, dot.lighten(0.2f)) + 12;
    D::textIn(Face::MonoBold, 12, Rect(tx2, chip.y, 200, chip.h), info, theme::textDim);
}

void App::drawControlCard(const Rect& r) {
    UI& ui = UI::get();
    Rect c = ui.card(r, "CONTROL", theme::green, sim_.cu->name() + " control unit");
    Rect row = c.cutTop(44);
    float bw = 40, runW = row.w - 4 * bw - 4 * 8;
    float x = row.x;
    if (ui.iconButton("back", Rect(x, row.y + 2, bw, 40), Icon::Back, "Step back  (\xE2\x86\x90 / F9)", Style::Secondary,
                      !sim_.history.empty()))
        sim_.stepBack();
    x += bw + 8;
    if (ui.iconButton("step", Rect(x, row.y + 2, bw, 40), Icon::Step, "Step one instruction  (\xE2\x86\x92 / F10)",
                      Style::Secondary, sim_.loaded()))
        sim_.step();
    x += bw + 8;
    if (ui.button("run", Rect(x, row.y, runW, 44), sim_.running ? "Pause" : "Run", Style::Primary,
                  int(sim_.running ? Icon::Pause : Icon::Play)))
        sim_.toggleRun();
    if (ui.hovered(Rect(x, row.y, runW, 44))) ui.tooltip("Run / pause  (Space)");
    x += runW + 8;
    if (ui.iconButton("finish", Rect(x, row.y + 2, bw, 40), Icon::Finish, "Run to the end (no animation)",
                      Style::Secondary, sim_.loaded()))
        sim_.finish();
    x += bw + 8;
    if (ui.iconButton("reset", Rect(x, row.y + 2, bw, 40), Icon::Reset, "Reset the CPU", Style::Secondary, sim_.loaded()))
        sim_.reset();
    c.cutTop(10);
    Rect seg = c.cutTop(32);
    int cur = sim_.micro ? 1 : 0;
    int nv = ui.segmented("cu", seg, {"Hardwired", "Microprogrammed"}, cur, theme::violet);
    if (nv != cur) sim_.setMicro(nv == 1);
    if (ui.hovered(seg)) ui.tooltip("Control unit organisation (COA CO4)");
    c.cutTop(10);
    Rect sp = c.cutTop(24);
    D::textIn(Face::Semibold, 12, Rect(sp.x, sp.y, 60, sp.h), "Speed", theme::textDim);
    char b[16];
    std::snprintf(b, sizeof b, "%.1fx", sim_.speed);
    D::textIn(Face::MonoBold, 12, Rect(sp.r() - 44, sp.y, 44, sp.h), b, theme::text, Align::Right);
    ui.slider("speed", Rect(sp.x + 58, sp.y, sp.w - 58 - 56, sp.h), sim_.speed, 0.25f, 12.f);
}

void App::drawStatusBar(const Rect& r) {
    D::rect(r, theme::bg1.alpha(0.95f));
    D::rect(Rect(r.x, r.y, r.w, 1), theme::border);
    Color dot = sim_.running ? theme::green : (sim_.loaded() ? (sim_.cpu.halted() ? theme::amber : theme::cyan) : theme::textMute);
    std::string state = sim_.running ? "RUNNING" : (!sim_.loaded() ? "NO PROGRAM" : (sim_.cpu.halted() ? "HALTED" : "READY"));
    D::circle(Vec2(r.x + 20, r.center().y), 4, dot);
    D::circle(Vec2(r.x + 20, r.center().y), 8, dot.alpha(0.2f));
    D::textIn(Face::Bold, 11, Rect(r.x + 32, r.y, 100, r.h), state, dot);
    D::textIn(Face::Regular, 12, Rect(r.x + 120, r.y, r.w * 0.55f, r.h),
              D::ellipsize(Face::Regular, 12, sim_.status, r.w * 0.55f), theme::textDim);
    char b[200];
    const char* hm = sim_.hashMethod == ds::HashMethod::Linear ? "Linear probing"
                   : sim_.hashMethod == ds::HashMethod::Quadratic ? "Quadratic probing" : "Chaining";
    std::snprintf(b, sizeof b, "cycle %ld    \xC2\xB7    instructions %ld    \xC2\xB7    %s CU    \xC2\xB7    symbols: %s    \xC2\xB7    Ctrl+S save  Ctrl+E export trace",
                  core::Clock::instance().cycle(), sim_.cpu.steps(), sim_.cu->name().c_str(), hm);
    D::textIn(Face::Regular, 12, r, b, theme::textMute, Align::Right, 16);
}

void App::drawToast() {
    if (sim_.toastTime <= 0) return;
    float a = std::min(1.f, sim_.toastTime / 0.4f) * std::min(1.f, (5.0f - sim_.toastTime) / 0.25f + 0.2f);
    a = clampf(a, 0, 1);
    float w = std::max(D::textWidth(Face::Regular, 13, sim_.toastBody), D::textWidth(Face::Bold, 14, sim_.toastTitle)) + 80;
    w = std::min(w, float(w_) * 0.6f);
    Rect r(w_ / 2.f - w / 2, 78 - (1 - a) * 12, w, 62);
    Color c = sim_.toastError ? theme::red : theme::green;
    D::shadow(r, 14, 22, Color(0, 0, 0, a));
    D::roundRect(r, 14, theme::overlay.alpha(0.97f * a));
    D::roundRectOutline(r, 14, c.alpha(0.6f * a));
    D::rect(Rect(r.x, r.y + 12, 3, r.h - 24), c.alpha(a));
    D::icon(sim_.toastError ? Icon::Cross : Icon::Check, Rect(r.x + 18, r.y + 21, 20, 20), c.alpha(a));
    D::text(Face::Bold, 14, r.x + 50, r.y + 11, sim_.toastTitle, theme::text.alpha(a));
    D::text(Face::Regular, 13, r.x + 50, r.y + 33, D::ellipsize(Face::Regular, 13, sim_.toastBody, r.w - 64),
            theme::textDim.alpha(a));
}

// Minimal PNG writer (stored / uncompressed deflate blocks) - no libraries needed.
static void writePng(const std::string& path, const std::vector<unsigned char>& rgbBottomUp, int w, int h) {
    static unsigned crcTable[256];
    static bool init = false;
    if (!init) {
        for (unsigned n = 0; n < 256; ++n) {
            unsigned c = n;
            for (int k = 0; k < 8; ++k) c = (c & 1) ? 0xEDB88320u ^ (c >> 1) : c >> 1;
            crcTable[n] = c;
        }
        init = true;
    }
    auto be32 = [](std::vector<unsigned char>& v, unsigned x) {
        v.push_back((x >> 24) & 255); v.push_back((x >> 16) & 255); v.push_back((x >> 8) & 255); v.push_back(x & 255);
    };
    auto chunk = [&](std::vector<unsigned char>& out, const char* type, const std::vector<unsigned char>& data) {
        be32(out, unsigned(data.size()));
        std::vector<unsigned char> td(type, type + 4);
        td.insert(td.end(), data.begin(), data.end());
        unsigned c = 0xFFFFFFFFu;
        for (unsigned char b : td) c = crcTable[(c ^ b) & 255] ^ (c >> 8);
        out.insert(out.end(), td.begin(), td.end());
        be32(out, c ^ 0xFFFFFFFFu);
    };
    // raw scanlines (top-down), filter byte 0
    std::vector<unsigned char> raw;
    raw.reserve(std::size_t(h) * (w * 3 + 1));
    for (int y = h - 1; y >= 0; --y) {
        raw.push_back(0);
        raw.insert(raw.end(), rgbBottomUp.begin() + std::size_t(y) * w * 3, rgbBottomUp.begin() + std::size_t(y + 1) * w * 3);
    }
    std::vector<unsigned char> z = {0x78, 0x01};
    unsigned a = 1, b = 0;
    for (std::size_t pos = 0; pos < raw.size();) {
        std::size_t len = std::min<std::size_t>(65535, raw.size() - pos);
        z.push_back(pos + len == raw.size() ? 1 : 0);
        z.push_back(len & 255); z.push_back(len >> 8);
        z.push_back(~len & 255); z.push_back((~len >> 8) & 255);
        for (std::size_t i = 0; i < len; ++i) {
            unsigned char ch = raw[pos + i];
            z.push_back(ch);
            a = (a + ch) % 65521;
            b = (b + a) % 65521;
        }
        pos += len;
    }
    be32(z, (b << 16) | a);
    std::vector<unsigned char> png = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};
    std::vector<unsigned char> ihdr;
    be32(ihdr, unsigned(w));
    be32(ihdr, unsigned(h));
    ihdr.insert(ihdr.end(), {8, 2, 0, 0, 0});
    chunk(png, "IHDR", ihdr);
    chunk(png, "IDAT", z);
    chunk(png, "IEND", {});
    FILE* f = std::fopen(path.c_str(), "wb");
    if (!f) return;
    std::fwrite(png.data(), 1, png.size(), f);
    std::fclose(f);
}

void App::saveScreenshot() {
    std::vector<unsigned char> px(std::size_t(w_) * h_ * 3);
    glReadBuffer(GL_BACK);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, w_, h_, GL_RGB, GL_UNSIGNED_BYTE, px.data());
    if (shotPath_.size() > 4 && shotPath_.substr(shotPath_.size() - 4) == ".png") {
        writePng(shotPath_, px, w_, h_);
        return;
    }
    FILE* f = std::fopen(shotPath_.c_str(), "wb");
    if (!f) return;
    std::fprintf(f, "P6\n%d %d\n255\n", w_, h_);
    for (int y = h_ - 1; y >= 0; --y) std::fwrite(&px[std::size_t(y) * w_ * 3], 1, std::size_t(w_) * 3, f);
    std::fclose(f);
}

} // namespace vas::ui
