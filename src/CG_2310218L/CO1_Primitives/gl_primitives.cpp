// Draws points, lines, triangles, quads and polygons with the OpenGL primitive types.

#include <GL/gl.h>

#include <cmath>
#include <vector>

struct Point2 {
    float x;
    float y;
};

// GL_POINTS: every vertex is one square dot of the given size in pixels
void drawPoints(const std::vector<Point2>& pts, float size) {
    glPointSize(size);
    glBegin(GL_POINTS);
    for (std::size_t i = 0; i < pts.size(); ++i) {
        glVertex2f(pts[i].x, pts[i].y);
    }
    glEnd();
}

// GL_LINES: every two vertices make one separate segment
void drawLines(const std::vector<Point2>& pts, float width) {
    glLineWidth(width);
    glBegin(GL_LINES);
    for (std::size_t i = 0; i + 1 < pts.size(); i += 2) {
        glVertex2f(pts[i].x, pts[i].y);
        glVertex2f(pts[i + 1].x, pts[i + 1].y);
    }
    glEnd();
}

// GL_LINE_STRIP: each vertex is joined to the one before it, giving one connected path
void drawLineStrip(const std::vector<Point2>& pts, float width) {
    glLineWidth(width);
    glBegin(GL_LINE_STRIP);
    for (std::size_t i = 0; i < pts.size(); ++i) {
        glVertex2f(pts[i].x, pts[i].y);
    }
    glEnd();
}

// GL_LINE_LOOP: like a strip, and the last vertex is joined back to the first (an outline)
void drawLineLoop(const std::vector<Point2>& pts, float width) {
    glLineWidth(width);
    glBegin(GL_LINE_LOOP);
    for (std::size_t i = 0; i < pts.size(); ++i) {
        glVertex2f(pts[i].x, pts[i].y);
    }
    glEnd();
}

// GL_TRIANGLES: one filled triangle, with a different colour at each corner blended across it
void drawTriangle(Point2 a, Point2 b, Point2 c, const float colorA[3], const float colorB[3], const float colorC[3]) {
    glBegin(GL_TRIANGLES);
    glColor3fv(colorA);
    glVertex2f(a.x, a.y);
    glColor3fv(colorB);
    glVertex2f(b.x, b.y);
    glColor3fv(colorC);
    glVertex2f(c.x, c.y);
    glEnd();
}

// GL_TRIANGLE_STRIP: the vertices zig-zag between two rails, so a band of n quads needs only 2n + 2 vertices
void drawBand(const std::vector<Point2>& top, const std::vector<Point2>& bottom) {
    glBegin(GL_TRIANGLE_STRIP);
    for (std::size_t i = 0; i < top.size() && i < bottom.size(); ++i) {
        glVertex2f(top[i].x, top[i].y);
        glVertex2f(bottom[i].x, bottom[i].y);
    }
    glEnd();
}

// GL_TRIANGLE_FAN: every triangle shares the centre, which makes a filled disc out of a ring of vertices
void drawDisc(Point2 center, float radius, int segments) {
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(center.x, center.y);
    for (int i = 0; i <= segments; ++i) {
        float angle = 2.0f * 3.14159265f * static_cast<float>(i) / static_cast<float>(segments);
        glVertex2f(center.x + radius * std::cos(angle), center.y + radius * std::sin(angle));
    }
    glEnd();
}

// GL_QUADS: four vertices in order around the corners make one rectangle
void drawQuad(Point2 a, Point2 b, Point2 c, Point2 d) {
    glBegin(GL_QUADS);
    glVertex2f(a.x, a.y);
    glVertex2f(b.x, b.y);
    glVertex2f(c.x, c.y);
    glVertex2f(d.x, d.y);
    glEnd();
}

// GL_POLYGON: a convex polygon with any number of corners, listed in order
void drawPolygon(const std::vector<Point2>& corners) {
    glBegin(GL_POLYGON);
    for (std::size_t i = 0; i < corners.size(); ++i) {
        glVertex2f(corners[i].x, corners[i].y);
    }
    glEnd();
}
