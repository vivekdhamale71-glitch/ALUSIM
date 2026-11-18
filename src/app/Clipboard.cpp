#include "app/Clipboard.h"

#include <cstdio>
#include <cstring>

#ifdef _WIN32
#  include <windows.h>
#else
#  include <X11/Xatom.h>
#  include <X11/Xlib.h>
#  include <unistd.h>

#  include <chrono>
#endif

namespace vas::ui {

std::string sanitizePaste(const std::string& raw, std::size_t maxLen) {
    std::string out;
    out.reserve(raw.size());
    for (std::size_t i = 0; i < raw.size() && out.size() < maxLen; ++i) {
        unsigned char c = static_cast<unsigned char>(raw[i]);
        if (c == '\r') {
            if (i + 1 < raw.size() && raw[i + 1] == '\n') ++i;   // CRLF counts once
            out += '\n';
        } else if (c == '\n') {
            out += '\n';
        } else if (c == '\t') {
            out += "    ";
        } else if (c >= 32 && c < 127) {
            out += char(c);
        } else if (c == 0xC2 && i + 1 < raw.size() && static_cast<unsigned char>(raw[i + 1]) == 0xA0) {
            out += ' ';                                           // non-breaking space
            ++i;
        }                                                         // anything else is dropped
    }
    return out;
}

#ifdef _WIN32
std::string clipboardText() {
    std::string out;
    if (!OpenClipboard(nullptr)) return out;
    if (HANDLE h = GetClipboardData(CF_TEXT)) {
        if (const char* p = static_cast<const char*>(GlobalLock(h))) {
            out = p;
            GlobalUnlock(h);
        }
    }
    CloseClipboard();
    return out;
}
#else

namespace {

// runs a command and returns its output ("" if it is missing or fails)
std::string run(const char* cmd) {
    std::string out;
    if (FILE* p = popen(cmd, "r")) {
        char buf[4096];
        std::size_t n;
        while ((n = std::fread(buf, 1, sizeof buf, p)) > 0 && out.size() < 4000000) out.append(buf, n);
        if (pclose(p) != 0) out.clear();
    }
    return out;
}

// Asks the owner of the CLIPBOARD selection for its text. A private hidden
// window receives the answer: owner -> SelectionNotify -> window property.
std::string x11Clipboard() {
    Display* d = XOpenDisplay(nullptr);
    if (!d) return {};
    Window w = XCreateSimpleWindow(d, DefaultRootWindow(d), 0, 0, 1, 1, 0, 0, 0);
    Atom clipboard = XInternAtom(d, "CLIPBOARD", False);
    Atom utf8 = XInternAtom(d, "UTF8_STRING", False);
    Atom prop = XInternAtom(d, "VAS_PASTE", False);
    Atom incr = XInternAtom(d, "INCR", False);

    std::string out;
    for (Atom target : {utf8, static_cast<Atom>(XA_STRING)}) {
        XConvertSelection(d, clipboard, target, prop, w, CurrentTime);
        XFlush(d);
        auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(450);
        bool answered = false;
        while (!answered && std::chrono::steady_clock::now() < deadline) {
            while (XPending(d)) {
                XEvent e;
                XNextEvent(d, &e);
                if (e.type != SelectionNotify) continue;
                answered = true;
                if (e.xselection.property == None) break;           // owner cannot give this format
                Atom type;
                int fmt;
                unsigned long n, left;
                unsigned char* data = nullptr;
                if (XGetWindowProperty(d, w, prop, 0, 1 << 20, True, AnyPropertyType, &type, &fmt, &n, &left, &data) == Success &&
                    data) {
                    if (type != incr && fmt == 8) out.assign(reinterpret_cast<char*>(data), n);   // INCR (huge text) is not supported
                    XFree(data);
                }
                break;
            }
            if (!answered) usleep(4000);
        }
        if (!out.empty()) break;
    }
    XDestroyWindow(d, w);
    XCloseDisplay(d);
    return out;
}

} // namespace

std::string clipboardText() {
    // a real Wayland / X11 clipboard tool is the most reliable source
    std::string t = run("wl-paste --no-newline 2>/dev/null");
    if (t.empty()) t = run("xclip -selection clipboard -o 2>/dev/null");
    if (t.empty()) t = run("xsel --clipboard --output 2>/dev/null");
    if (t.empty()) t = x11Clipboard();
    return t;
}
#endif

} // namespace vas::ui
