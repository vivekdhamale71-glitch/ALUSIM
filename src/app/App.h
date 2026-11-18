// Application shell: window, main loop, top bar, status bar, routing.
#pragma once
#include <memory>
#include <string>
#include <vector>

#include "app/Sim.h"
#include "app/UI.h"
#include "app/Views.h"

namespace vas::ui {

class App {
public:
    static App& get();
    void init(int argc, char** argv);
    void run();

    void display();
    void reshape(int w, int h);
    void keyboard(unsigned char key, int x, int y);
    void special(int key, int x, int y);
    void mouse(int button, int state, int x, int y);
    void motion(int x, int y);
    void wheel(int dir);

private:
    void drawBackground();
    void drawTopBar(const Rect& r);
    void drawControlCard(const Rect& r);
    void drawStatusBar(const Rect& r);
    void drawToast();
    void pasteFromClipboard();
    void syncSettings();               // Settings singleton -> settings.cfg
    void handleShortcuts();
    void saveScreenshot();

    Sim sim_;
    std::vector<std::unique_ptr<View>> views_;
    int tab_ = 0;
    int w_ = 1600, h_ = 920;
    int lastMs_ = 0;

    std::string shotPath_;
    int shotFrames_ = -1;
    int frame_ = 0;
    bool fixedStep_ = false;
    bool fullscreen_ = false;
    bool microscope_ = false;          // F8: a small square under the mouse, magnified in a larger square
    void drawMicroscope();
    bool useSettings_ = false;
    int savedW_ = 0, savedH_ = 0;
};

} // namespace vas::ui
