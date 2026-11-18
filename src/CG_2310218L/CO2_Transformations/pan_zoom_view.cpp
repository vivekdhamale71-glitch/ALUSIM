// A view matrix that pans and zooms about the mouse position, with its inverse.

#include <cmath>

struct Point2 {
    float x;
    float y;
};

// the view is "scale by zoom, then move by pan": screen = zoom * world + pan
struct View {
    float zoom;
    float panX;
    float panY;
};

// the starting view: no zoom and no movement
View startView() {
    View v = {1.0f, 0.0f, 0.0f};
    return v;
}

// world coordinates to screen coordinates
Point2 worldToScreen(const View& v, Point2 world) {
    Point2 s;
    s.x = world.x * v.zoom + v.panX;
    s.y = world.y * v.zoom + v.panY;
    return s;
}

// the inverse matrix: screen coordinates back to world coordinates (used to find what the mouse points at)
Point2 screenToWorld(const View& v, Point2 screen) {
    Point2 w;
    w.x = (screen.x - v.panX) / v.zoom;
    w.y = (screen.y - v.panY) / v.zoom;
    return w;
}

// dragging the mouse by (dx, dy) pixels moves the picture by the same amount
void panBy(View& v, float dx, float dy) {
    v.panX += dx;
    v.panY += dy;
}

// zooms by a factor while the world point under the mouse stays under the mouse:
// find that point, change the zoom, then choose the pan that puts the point back at the same pixel
void zoomAt(View& v, Point2 mouse, float factor, float minZoom, float maxZoom) {
    Point2 under = screenToWorld(v, mouse);
    float newZoom = v.zoom * factor;
    if (newZoom < minZoom) {
        newZoom = minZoom;
    }
    if (newZoom > maxZoom) {
        newZoom = maxZoom;
    }
    v.zoom = newZoom;
    v.panX = mouse.x - under.x * v.zoom;
    v.panY = mouse.y - under.y * v.zoom;
}

// changes the zoom smoothly: each frame the zoom moves a fraction of the way to the target
float easeToward(float current, float target, float speed, float seconds) {
    return current + (target - current) * (1.0f - std::exp(-speed * seconds));
}

// puts the whole drawing in the window: scale to fit, then centre it
View fitToWindow(float worldW, float worldH, float windowW, float windowH) {
    View v;
    float zx = windowW / worldW;
    float zy = windowH / worldH;
    v.zoom = zx;
    if (zy < zx) {
        v.zoom = zy;
    }
    v.panX = (windowW - worldW * v.zoom) / 2.0f;
    v.panY = (windowH - worldH * v.zoom) / 2.0f;
    return v;
}
