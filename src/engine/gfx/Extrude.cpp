#include "engine/gfx/Extrude.h"

#include "engine/gfx/GL.h"

namespace vas::gfx {

void prism(const std::vector<Vec2>& poly, float h, Vec2 fanCenter) {
    int n = int(poly.size());
    // side walls: one quad per edge, normal perpendicular to the edge
    glBegin(GL_QUADS);
    for (int i = 0; i < n; ++i) {
        Vec2 a = poly[i], b = poly[(i + 1) % n];
        Vec2 e = b - a;
        Vec2 nrm = Vec2(e.y, -e.x).normalized();
        glNormal3f(nrm.x, 0, nrm.y);
        glVertex3f(a.x, 0, a.y); glVertex3f(b.x, 0, b.y);
        glVertex3f(b.x, h, b.y); glVertex3f(a.x, h, a.y);
    }
    glEnd();
    // top cap
    glBegin(GL_TRIANGLE_FAN);
    glNormal3f(0, 1, 0);
    glVertex3f(fanCenter.x, h, fanCenter.y);
    for (int i = 0; i <= n; ++i) glVertex3f(poly[i % n].x, h, poly[i % n].y);
    glEnd();
}

void box(float x0, float z0, float x1, float z1, float y0, float y1) {
    std::vector<Vec2> ccw = {{x0, z0}, {x0, z1}, {x1, z1}, {x1, z0}};
    glPushMatrix();
    glTranslatef(0, y0, 0);
    prism(ccw, y1 - y0, {(x0 + x1) / 2, (z0 + z1) / 2});
    glPopMatrix();
}

} // namespace vas::gfx
