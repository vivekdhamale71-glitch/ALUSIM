// A CPU board with a register file, an ALU shape, bus wires and flag lamps drawn from primitives.

#include <GL/gl.h>

#include <cmath>
#include <vector>

struct Point2 {
    float x;
    float y;
};

// a filled rectangle in the given colour
static void box(float x, float y, float w, float h, float r, float g, float b) {
    glColor3f(r, g, b);
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();
}

// the green circuit board that everything else sits on
void drawBoard(float x, float y, float w, float h) {
    box(x, y, w, h, 0.05f, 0.14f, 0.12f);
    glColor3f(0.1f, 0.3f, 0.25f);
    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();
}

// the register file: one small box per register, stacked in a column; a register that holds a value is lit
void drawRegisterFile(float x, float y, int count, const bool lit[]) {
    for (int i = 0; i < count; ++i) {
        float top = y + static_cast<float>(i) * 22.0f;
        if (lit[i]) {
            box(x, top, 90.0f, 18.0f, 0.2f, 0.7f, 1.0f);
        } else {
            box(x, top, 90.0f, 18.0f, 0.1f, 0.2f, 0.3f);
        }
    }
}

// the ALU: the classic trapezoid with a notch at the top where the two operands come in
void drawAlu(float x, float y, float w, float h) {
    glColor3f(0.9f, 0.55f, 0.15f);
    glBegin(GL_POLYGON);
    glVertex2f(x, y);
    glVertex2f(x + w * 0.4f, y);
    glVertex2f(x + w * 0.5f, y + h * 0.2f);
    glVertex2f(x + w * 0.6f, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w * 0.8f, y + h);
    glVertex2f(x + w * 0.2f, y + h);
    glEnd();
}

// a bus wire that goes through several corners; a wide dim line under a thin bright one gives a glow
void drawBus(const std::vector<Point2>& corners, bool active) {
    glLineWidth(6.0f);
    glColor4f(0.2f, 0.6f, 1.0f, 0.25f);
    glBegin(GL_LINE_STRIP);
    for (std::size_t i = 0; i < corners.size(); ++i) {
        glVertex2f(corners[i].x, corners[i].y);
    }
    glEnd();
    glLineWidth(2.0f);
    if (active) {
        glColor3f(0.4f, 0.9f, 1.0f);
    } else {
        glColor3f(0.25f, 0.4f, 0.5f);
    }
    glBegin(GL_LINE_STRIP);
    for (std::size_t i = 0; i < corners.size(); ++i) {
        glVertex2f(corners[i].x, corners[i].y);
    }
    glEnd();
}

// a flag lamp (CF, ZF, SF, OF): a green disc when the flag is 1, a dark one when it is 0
void drawFlagLamp(float cx, float cy, float radius, bool on) {
    if (on) {
        glColor3f(0.2f, 0.95f, 0.5f);
    } else {
        glColor3f(0.08f, 0.2f, 0.15f);
    }
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for (int i = 0; i <= 24; ++i) {
        float angle = 2.0f * 3.14159265f * static_cast<float>(i) / 24.0f;
        glVertex2f(cx + radius * std::cos(angle), cy + radius * std::sin(angle));
    }
    glEnd();
}
