#include "CG_2310218L/CO4_ProjectionViewing/Scene3D.h"

#include <cmath>

#include "CG_2310218L/CO1_Primitives/Draw.h"
#include "CG_2310218L/CO1_Primitives/GL.h"

namespace vas::gfx::s3d {

static GLdouble gMv[16], gPj[16];
static GLint gView[4];
static int gScreenH = 1;

void begin(const Rect& vp, const Camera& cam, bool lightTheme) {
    gScreenH = draw::screenH();
    glViewport(int(vp.x), int(gScreenH - vp.b()), int(vp.w), int(vp.h));
    glEnable(GL_SCISSOR_TEST);
    glScissor(int(vp.x), int(gScreenH - vp.b()), int(vp.w), int(vp.h));
    glClear(GL_DEPTH_BUFFER_BIT);

    float aspect = vp.w / std::max(1.f, vp.h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(cam.fov, aspect, 0.3, 300);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    float yr = cam.yaw * float(M_PI) / 180, pr = cam.pitch * float(M_PI) / 180;
    gluLookAt(cam.tx + cam.dist * std::cos(pr) * std::sin(yr), cam.ty + cam.dist * std::sin(pr),
              cam.tz + cam.dist * std::cos(pr) * std::cos(yr), cam.tx, cam.ty, cam.tz, 0, 1, 0);
    glGetDoublev(GL_MODELVIEW_MATRIX, gMv);
    glGetDoublev(GL_PROJECTION_MATRIX, gPj);
    glGetIntegerv(GL_VIEWPORT, gView);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHT1);
    glEnable(GL_NORMALIZE);
    glShadeModel(GL_SMOOTH);
    GLfloat p0[4] = {-0.35f, 1.0f, 0.75f, 0};
    GLfloat d0[4] = {0.92f, 0.94f, 1.0f, 1};
    GLfloat a0[4] = {lightTheme ? 0.42f : 0.22f, lightTheme ? 0.44f : 0.25f, lightTheme ? 0.48f : 0.32f, 1};
    glLightfv(GL_LIGHT0, GL_POSITION, p0);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, d0);
    glLightfv(GL_LIGHT0, GL_SPECULAR, d0);
    glLightfv(GL_LIGHT0, GL_AMBIENT, a0);
    GLfloat p1[4] = {0.8f, 0.4f, -0.6f, 0};
    GLfloat d1[4] = {0.25f, 0.22f, 0.4f, 1};
    glLightfv(GL_LIGHT1, GL_POSITION, p1);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, d1);
    glLightfv(GL_LIGHT1, GL_SPECULAR, d1);
}

void end() {
    glDisable(GL_LIGHTING);
    glDisable(GL_LIGHT0);
    glDisable(GL_LIGHT1);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_NORMALIZE);
    glDisable(GL_SCISSOR_TEST);
    draw::beginFrame(draw::screenW(), draw::screenH());
}

void material(const Color& c, float emission, float shininess) {
    GLfloat amb[4] = {c.r * 0.45f, c.g * 0.45f, c.b * 0.45f, c.a};
    GLfloat dif[4] = {c.r * 0.9f, c.g * 0.9f, c.b * 0.9f, c.a};
    GLfloat spe[4] = {0.55f, 0.58f, 0.65f, 1};
    GLfloat emi[4] = {c.r * emission, c.g * emission, c.b * emission, 1};
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, amb);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, dif);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, spe);
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emi);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, shininess);
}

void box(float x0, float y0, float z0, float x1, float y1, float z1) {
    glBegin(GL_QUADS);
    glNormal3f(0, 0, 1);  glVertex3f(x0, y0, z1); glVertex3f(x1, y0, z1); glVertex3f(x1, y1, z1); glVertex3f(x0, y1, z1);
    glNormal3f(0, 0, -1); glVertex3f(x1, y0, z0); glVertex3f(x0, y0, z0); glVertex3f(x0, y1, z0); glVertex3f(x1, y1, z0);
    glNormal3f(1, 0, 0);  glVertex3f(x1, y0, z1); glVertex3f(x1, y0, z0); glVertex3f(x1, y1, z0); glVertex3f(x1, y1, z1);
    glNormal3f(-1, 0, 0); glVertex3f(x0, y0, z0); glVertex3f(x0, y0, z1); glVertex3f(x0, y1, z1); glVertex3f(x0, y1, z0);
    glNormal3f(0, 1, 0);  glVertex3f(x0, y1, z1); glVertex3f(x1, y1, z1); glVertex3f(x1, y1, z0); glVertex3f(x0, y1, z0);
    glNormal3f(0, -1, 0); glVertex3f(x0, y0, z0); glVertex3f(x1, y0, z0); glVertex3f(x1, y0, z1); glVertex3f(x0, y0, z1);
    glEnd();
}

void boxRotX(float cx, float cy, float cz, float sx, float sy, float sz, float angleDeg) {
    glPushMatrix();
    glTranslatef(cx, cy, cz);
    glRotatef(angleDeg, 1, 0, 0);
    box(-sx / 2, -sy / 2, -sz / 2, sx / 2, sy / 2, sz / 2);
    glPopMatrix();
}

void floorQuad(float x0, float z0, float x1, float z1, float y, const Color& c) {
    glDisable(GL_LIGHTING);
    glDepthMask(GL_FALSE);
    glColor4f(c.r, c.g, c.b, c.a);
    glBegin(GL_QUADS);
    glVertex3f(x0, y, z0); glVertex3f(x1, y, z0); glVertex3f(x1, y, z1); glVertex3f(x0, y, z1);
    glEnd();
    glDepthMask(GL_TRUE);
    glEnable(GL_LIGHTING);
}

bool project(float x, float y, float z, Vec2& out) {
    GLdouble wx, wy, wz;
    if (gluProject(x, y, z, gMv, gPj, gView, &wx, &wy, &wz) != GL_TRUE) return false;
    out = Vec2(float(wx), float(gScreenH - wy));
    return wz > 0 && wz < 1;
}

} // namespace vas::gfx::s3d
