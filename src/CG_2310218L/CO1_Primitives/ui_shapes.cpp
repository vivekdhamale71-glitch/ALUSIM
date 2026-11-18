// Rounded rectangles, gradients, outlines, circles, rings and dashed lines built from primitives.

#include <GL/gl.h>

#include <cmath>
#include <vector>

struct Point2 {
    float x;
    float y;
};

struct Rgba {
    float r;
    float g;
    float b;
    float a;
};

// switches on alpha blending so that colours with a < 1 are see-through
void enableBlending() {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

// sets the current drawing colour from a struct
void setColor(const Rgba& c) {
    glColor4f(c.r, c.g, c.b, c.a);
}

// one filled rectangle as a quad
void fillRect(float x, float y, float w, float h, const Rgba& c) {
    setColor(c);
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();
}

// a vertical gradient: the corners at the top get one colour, the corners at the bottom another, OpenGL blends between them
void gradientRect(float x, float y, float w, float h, const Rgba& top, const Rgba& bottom) {
    glBegin(GL_QUADS);
    setColor(top);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    setColor(bottom);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();
}

// the outline of a rounded rectangle: a straight run on each side and a quarter circle at each corner
std::vector<Point2> roundedPath(float x, float y, float w, float h, float radius, int arcSegments) {
    std::vector<Point2> path;
    float cx[4] = {x + w - radius, x + w - radius, x + radius, x + radius};
    float cy[4] = {y + radius, y + h - radius, y + h - radius, y + radius};
    for (int corner = 0; corner < 4; ++corner) {
        for (int i = 0; i <= arcSegments; ++i) {
            float angle = (static_cast<float>(corner) * 90.0f - 90.0f + 90.0f * static_cast<float>(i) / static_cast<float>(arcSegments)) * 3.14159265f / 180.0f;
            Point2 p;
            p.x = cx[corner] + radius * std::cos(angle);
            p.y = cy[corner] + radius * std::sin(angle);
            path.push_back(p);
        }
    }
    return path;
}

// a filled rounded rectangle: a triangle fan from the centre to every point of the outline
void fillRoundedRect(float x, float y, float w, float h, float radius, const Rgba& c) {
    std::vector<Point2> path = roundedPath(x, y, w, h, radius, 6);
    setColor(c);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(x + w / 2.0f, y + h / 2.0f);
    for (std::size_t i = 0; i < path.size(); ++i) {
        glVertex2f(path[i].x, path[i].y);
    }
    glVertex2f(path[0].x, path[0].y);
    glEnd();
}

// the border of a rounded rectangle drawn as a closed line loop
void outlineRoundedRect(float x, float y, float w, float h, float radius, const Rgba& c, float width) {
    std::vector<Point2> path = roundedPath(x, y, w, h, radius, 6);
    setColor(c);
    glLineWidth(width);
    glBegin(GL_LINE_LOOP);
    for (std::size_t i = 0; i < path.size(); ++i) {
        glVertex2f(path[i].x, path[i].y);
    }
    glEnd();
}

// a filled circle as a triangle fan
void fillCircle(Point2 center, float radius, const Rgba& c, int segments) {
    setColor(c);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(center.x, center.y);
    for (int i = 0; i <= segments; ++i) {
        float angle = 2.0f * 3.14159265f * static_cast<float>(i) / static_cast<float>(segments);
        glVertex2f(center.x + radius * std::cos(angle), center.y + radius * std::sin(angle));
    }
    glEnd();
}

// a ring (a circle with a hole): a triangle strip between an inner and an outer circle
void drawRing(Point2 center, float radius, float thickness, const Rgba& c, int segments) {
    setColor(c);
    glBegin(GL_TRIANGLE_STRIP);
    for (int i = 0; i <= segments; ++i) {
        float angle = 2.0f * 3.14159265f * static_cast<float>(i) / static_cast<float>(segments);
        float cs = std::cos(angle);
        float sn = std::sin(angle);
        glVertex2f(center.x + (radius - thickness / 2.0f) * cs, center.y + (radius - thickness / 2.0f) * sn);
        glVertex2f(center.x + (radius + thickness / 2.0f) * cs, center.y + (radius + thickness / 2.0f) * sn);
    }
    glEnd();
}

// a dashed line: walk along the segment and draw a short line for each dash, skipping the gaps
void dashedLine(Point2 a, Point2 b, const Rgba& c, float width, float dash, float gap, float offset) {
    float dx = b.x - a.x;
    float dy = b.y - a.y;
    float length = std::sqrt(dx * dx + dy * dy);
    if (length < 0.001f) {
        return;
    }
    dx = dx / length;
    dy = dy / length;
    setColor(c);
    glLineWidth(width);
    glBegin(GL_LINES);
    float start = -offset;
    while (start < length) {
        float from = start;
        float to = start + dash;
        if (from < 0.0f) {
            from = 0.0f;
        }
        if (to > length) {
            to = length;
        }
        if (to > from) {
            glVertex2f(a.x + dx * from, a.y + dy * from);
            glVertex2f(a.x + dx * to, a.y + dy * to);
        }
        start += dash + gap;
    }
    glEnd();
}
