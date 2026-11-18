// A 2D outline swept into a 3D solid, and a profile revolved into a vase.

#include <GL/gl.h>

#include <cmath>
#include <vector>

struct Point2 {
    float x;
    float y;
};

// extrusion: the outline is copied up by a height; each edge becomes a wall (a quad)
// and a triangle fan closes the top. The outline must be listed counter-clockwise.
void extrude(const std::vector<Point2>& outline, float height, Point2 capCenter) {
    int n = static_cast<int>(outline.size());
    glBegin(GL_QUADS);
    for (int i = 0; i < n; ++i) {
        Point2 a = outline[i];
        Point2 b = outline[(i + 1) % n];
        float ex = b.x - a.x;
        float ey = b.y - a.y;
        float length = std::sqrt(ex * ex + ey * ey);
        glNormal3f(ey / length, 0.0f, -ex / length);     // the wall faces away from the outline
        glVertex3f(a.x, 0.0f, a.y);
        glVertex3f(b.x, 0.0f, b.y);
        glVertex3f(b.x, height, b.y);
        glVertex3f(a.x, height, a.y);
    }
    glEnd();
    glBegin(GL_TRIANGLE_FAN);
    glNormal3f(0.0f, 1.0f, 0.0f);
    glVertex3f(capCenter.x, height, capCenter.y);
    for (int i = 0; i <= n; ++i) {
        glVertex3f(outline[i % n].x, height, outline[i % n].y);
    }
    glEnd();
}

// a box is an extrusion of a rectangle
void drawBox(float x0, float z0, float x1, float z1, float y0, float y1) {
    std::vector<Point2> rectangle;
    Point2 corners[4] = {{x0, z0}, {x0, z1}, {x1, z1}, {x1, z0}};
    for (int i = 0; i < 4; ++i) {
        rectangle.push_back(corners[i]);
    }
    Point2 center = {(x0 + x1) / 2.0f, (z0 + z1) / 2.0f};
    glPushMatrix();
    glTranslatef(0.0f, y0, 0.0f);
    extrude(rectangle, y1 - y0, center);
    glPopMatrix();
}

// surface of revolution: the profile (radius at each height) is turned around the y axis;
// every pair of neighbouring profile points and neighbouring angles makes one quad
void lathe(const std::vector<Point2>& profile, int slices) {
    for (std::size_t i = 0; i + 1 < profile.size(); ++i) {
        glBegin(GL_QUAD_STRIP);
        for (int s = 0; s <= slices; ++s) {
            float angle = 2.0f * 3.14159265f * static_cast<float>(s) / static_cast<float>(slices);
            float cs = std::cos(angle);
            float sn = std::sin(angle);
            glNormal3f(cs, 0.0f, sn);
            glVertex3f(profile[i].x * cs, profile[i].y, profile[i].x * sn);
            glVertex3f(profile[i + 1].x * cs, profile[i + 1].y, profile[i + 1].x * sn);
        }
        glEnd();
    }
}

// the profile of a vase: narrow foot, wide belly, narrow neck, small lip
std::vector<Point2> vaseProfile() {
    std::vector<Point2> profile;
    Point2 points[7] = {{0.4f, 0.0f}, {0.9f, 0.4f}, {1.1f, 1.0f}, {0.8f, 1.7f}, {0.4f, 2.1f}, {0.45f, 2.4f}, {0.6f, 2.5f}};
    for (int i = 0; i < 7; ++i) {
        profile.push_back(points[i]);
    }
    return profile;
}
