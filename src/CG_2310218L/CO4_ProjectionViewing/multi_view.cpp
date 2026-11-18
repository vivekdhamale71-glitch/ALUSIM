// Four viewports showing the front, top, side and perspective views of one scene.

#include <GL/gl.h>
#include <GL/glu.h>

// the scene drawn in every view; supplied by the caller
typedef void (*SceneFunction)();

struct ViewRect {
    int x;
    int y;
    int w;
    int h;
};

// restricts all drawing to a rectangle of the window: the viewport maps the picture into it,
// and the scissor test stops anything (including the clear) from touching the rest
void beginViewport(const ViewRect& r) {
    glViewport(r.x, r.y, r.w, r.h);
    glEnable(GL_SCISSOR_TEST);
    glScissor(r.x, r.y, r.w, r.h);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

// ends the view so the next one can set its own
void endViewport() {
    glDisable(GL_SCISSOR_TEST);
}

// a parallel projection that looks along one axis, with the given up direction
static void parallelView(const ViewRect& r, float eyeX, float eyeY, float eyeZ, float upX, float upY, float upZ, float half) {
    float aspect = static_cast<float>(r.w) / static_cast<float>(r.h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-half * aspect, half * aspect, -half, half, -100.0, 100.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(eyeX, eyeY, eyeZ, 0, 0, 0, upX, upY, upZ);
}

// front view: looking along -z, so x is across and y is up
void drawFront(const ViewRect& r, SceneFunction scene) {
    beginViewport(r);
    parallelView(r, 0, 0, 10, 0, 1, 0, 6.0f);
    scene();
    endViewport();
}

// top view: looking down along -y, so x is across and z is down the screen
void drawTop(const ViewRect& r, SceneFunction scene) {
    beginViewport(r);
    parallelView(r, 0, 10, 0, 0, 0, -1, 6.0f);
    scene();
    endViewport();
}

// side view: looking along -x, so z is across and y is up
void drawSide(const ViewRect& r, SceneFunction scene) {
    beginViewport(r);
    parallelView(r, 10, 0, 0, 0, 1, 0, 6.0f);
    scene();
    endViewport();
}

// perspective view: the way the eye would see the scene from a corner
void drawPerspective(const ViewRect& r, SceneFunction scene) {
    beginViewport(r);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(40.0, static_cast<double>(r.w) / static_cast<double>(r.h), 0.5, 100.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(7, 6, 9, 0, 0, 0, 0, 1, 0);
    scene();
    endViewport();
}

// the four views are the four quarters of the window (OpenGL's y grows upwards, so the top row starts at half the height)
void drawFourViews(int windowW, int windowH, SceneFunction scene) {
    int halfW = windowW / 2;
    int halfH = windowH / 2;
    ViewRect topLeft = {0, halfH, halfW, halfH};
    ViewRect topRight = {halfW, halfH, windowW - halfW, halfH};
    ViewRect bottomLeft = {0, 0, halfW, windowH - halfH};
    ViewRect bottomRight = {halfW, 0, windowW - halfW, windowH - halfH};
    glEnable(GL_DEPTH_TEST);
    drawFront(topLeft, scene);
    drawTop(topRight, scene);
    drawSide(bottomLeft, scene);
    drawPerspective(bottomRight, scene);
}
