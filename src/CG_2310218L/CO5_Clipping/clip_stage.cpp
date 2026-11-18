// A clipping stage that accepts, rejects or clips every shape against a window.

#include <GL/gl.h>

#include <algorithm>
#include <vector>

struct Vertex {
    float x;
    float y;
    float r;
    float g;
    float b;
    float a;
};

struct Window {
    float left;
    float top;
    float right;
    float bottom;
};

// how many shapes were drawn as they are, thrown away, and cut
struct ClipStats {
    int accepted;
    int rejected;
    int clipped;
};

// the polygon clipper (Sutherland-Hodgman) written in polygon_clipping.cpp
std::vector<Vertex> sutherlandHodgman(const std::vector<Vertex>& polygon, const Window& w);

// the window and counters used by the stage
static bool hasWindow = false;
static Window currentWindow = {0, 0, 0, 0};
static ClipStats stats = {0, 0, 0};

// sets the window that all later shapes are clipped against
void setWindow(const Window& w) {
    hasWindow = true;
    currentWindow = w;
}

// removes the window: shapes are drawn unclipped
void clearWindow() {
    hasWindow = false;
}

// puts the counters back to zero at the start of a frame
void resetStats() {
    stats.accepted = 0;
    stats.rejected = 0;
    stats.clipped = 0;
}

// reads the counters
ClipStats readStats() {
    return stats;
}

// draws a polygon as a triangle fan, with the colour of each corner
static void emit(const std::vector<Vertex>& polygon) {
    glBegin(GL_TRIANGLE_FAN);
    for (std::size_t i = 0; i < polygon.size(); ++i) {
        glColor4f(polygon[i].r, polygon[i].g, polygon[i].b, polygon[i].a);
        glVertex2f(polygon[i].x, polygon[i].y);
    }
    glEnd();
}

// the stage: compare the bounding box of the shape with the window
//   box inside: accept and draw as it is;  box completely outside: reject, nothing is drawn;
//   otherwise: clip the polygon and draw what is left
void drawClipped(const std::vector<Vertex>& polygon) {
    if (polygon.size() < 3) {
        return;
    }
    if (!hasWindow) {
        emit(polygon);
        return;
    }
    float minX = polygon[0].x;
    float maxX = polygon[0].x;
    float minY = polygon[0].y;
    float maxY = polygon[0].y;
    for (std::size_t i = 1; i < polygon.size(); ++i) {
        minX = std::min(minX, polygon[i].x);
        maxX = std::max(maxX, polygon[i].x);
        minY = std::min(minY, polygon[i].y);
        maxY = std::max(maxY, polygon[i].y);
    }
    bool inside = minX >= currentWindow.left && maxX <= currentWindow.right && minY >= currentWindow.top && maxY <= currentWindow.bottom;
    if (inside) {
        ++stats.accepted;
        emit(polygon);
        return;
    }
    bool outside = maxX < currentWindow.left || minX > currentWindow.right || maxY < currentWindow.top || minY > currentWindow.bottom;
    if (outside) {
        ++stats.rejected;
        return;
    }
    std::vector<Vertex> cut = sutherlandHodgman(polygon, currentWindow);
    ++stats.clipped;
    if (cut.size() >= 3) {
        emit(cut);
    }
}

// a rectangle goes through the same stage as a four-corner polygon
void drawClippedRect(float x, float y, float w, float h, float r, float g, float b, float a) {
    std::vector<Vertex> corners;
    Vertex c0 = {x, y, r, g, b, a};
    Vertex c1 = {x + w, y, r, g, b, a};
    Vertex c2 = {x + w, y + h, r, g, b, a};
    Vertex c3 = {x, y + h, r, g, b, a};
    corners.push_back(c0);
    corners.push_back(c1);
    corners.push_back(c2);
    corners.push_back(c3);
    drawClipped(corners);
}
