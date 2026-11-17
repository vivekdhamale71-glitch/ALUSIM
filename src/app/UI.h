// =====================================================================
//  Immediate-mode widget toolkit built on gfx::draw.
//  Every widget is a function call that draws itself and returns what
//  the user did this frame; hover / press transitions are animated.
// =====================================================================
#pragma once
#include <functional>
#include <map>
#include <string>
#include <vector>

#include "CG_2310218L/CO1_Primitives/Draw.h"
#include "app/Theme.h"

namespace vas::ui {

using gfx::Color;
using gfx::Rect;
using gfx::Vec2;
using gfx::draw::Icon;

enum Key { K_LEFT = 1000, K_RIGHT, K_UP, K_DOWN, K_HOME, K_END, K_PGUP, K_PGDN, K_F1, K_F2, K_F3, K_F4, K_F5, K_F6,
           K_F7, K_F8, K_F9, K_F10, K_F11, K_F12, K_DELETE };

struct Input {
    Vec2 mouse;
    bool down = false, pressed = false, released = false;
    bool rdown = false;
    float wheel = 0;
    std::vector<int> keys;     // chars (ASCII) and Key codes typed this frame
    bool ctrl = false, shift = false, alt = false;
};

enum class Style { Primary, Secondary, Ghost, Danger, Success };

struct EditorState {
    int cursor = 0;
    float scroll = 0;
    bool focused = false;
    float blink = 0;
    int preferredCol = -1;
};

struct HighlightSpan {
    int start, end;
    Color color;
};
using Highlighter = std::function<std::vector<HighlightSpan>(const std::string& line)>;

class UI {
public:
    static UI& get();

    void beginFrame(float dt);
    void endFrame();

    Input in;
    float dt = 0.016f;
    float time = 0;
    bool keyboardTaken = false;   // an editor consumed the keys this frame

    bool hovered(const Rect& r) const { return r.contains(in.mouse) && !mouseBlocked_; }
    bool clicked(const Rect& r) const { return in.released && r.contains(in.mouse) && !mouseBlocked_; }
    void blockMouse(bool b) { mouseBlocked_ = b; }
    float anim(const std::string& id, float target, float speed = 14.0f);

    // widgets
    bool button(const std::string& id, const Rect& r, const std::string& label, Style st = Style::Secondary,
                int icon = -1, bool enabled = true);
    bool iconButton(const std::string& id, const Rect& r, Icon icon, const std::string& tip,
                    Style st = Style::Ghost, bool enabled = true);
    int segmented(const std::string& id, const Rect& r, const std::vector<std::string>& opts, int cur,
                  const Color& accent = theme::cyan);
    bool toggle(const std::string& id, const Rect& r, bool& v, const std::string& label);
    bool slider(const std::string& id, const Rect& r, float& v, float lo, float hi);
    bool stepper(const std::string& id, const Rect& r, int& v, int lo, int hi, const std::string& label);

    // decoration
    Rect card(const Rect& r, const std::string& title, const Color& accent = theme::cyan, const std::string& right = "");
    void pill(const Rect& r, const std::string& s, const Color& c, bool filled = false, int px = 12);
    float chip(float x, float y, const std::string& s, const Color& c, bool on, int px = 11);
    void tooltip(const std::string& s);
    void emptyState(const Rect& r, const std::string& title, const std::string& body);

    bool codeEditor(const std::string& id, const Rect& r, std::string& text, EditorState& st, const Highlighter& hl,
                    int activeLine = -1, int errorLine = -1);

private:
    std::map<std::string, float> anims_;
    std::string pressedId_;
    std::string tip_;
    Vec2 tipPos_;
    float tipTimer_ = 0;
    std::string lastTipId_;
    bool mouseBlocked_ = false;
};

// splits text into lines
std::vector<std::string> splitLines(const std::string& s);

} // namespace vas::ui
