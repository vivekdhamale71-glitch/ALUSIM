#include "engine/gfx/Lighting.h"

#include "engine/gfx/GL.h"

namespace vas::gfx {

void material(const Color& c, const Color& emission, float shininess) {
    GLfloat amb[4] = {c.r * 0.45f, c.g * 0.45f, c.b * 0.45f, 1};
    GLfloat dif[4] = {c.r, c.g, c.b, 1};
    GLfloat spe[4] = {0.55f, 0.6f, 0.7f, 1};
    GLfloat emi[4] = {emission.r, emission.g, emission.b, 1};
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, amb);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, dif);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, spe);
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emi);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, shininess);
}

void chipLights(const Color& phase) {
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHT1);
    glEnable(GL_NORMALIZE);
    glShadeModel(GL_SMOOTH);
    GLfloat l0pos[4] = {-0.4f, 1.0f, 0.6f, 0};          // w = 0: directional
    GLfloat l0dif[4] = {0.9f, 0.92f, 1.0f, 1};
    GLfloat l0amb[4] = {0.25f, 0.28f, 0.35f, 1};
    glLightfv(GL_LIGHT0, GL_POSITION, l0pos);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, l0dif);
    glLightfv(GL_LIGHT0, GL_SPECULAR, l0dif);
    glLightfv(GL_LIGHT0, GL_AMBIENT, l0amb);
    GLfloat l1pos[4] = {6, 5, -4, 1};                    // w = 1: point light
    GLfloat l1dif[4] = {phase.r * 0.6f, phase.g * 0.6f, phase.b * 0.6f, 1};
    glLightfv(GL_LIGHT1, GL_POSITION, l1pos);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, l1dif);
    glLightfv(GL_LIGHT1, GL_SPECULAR, l1dif);
    glLightf(GL_LIGHT1, GL_QUADRATIC_ATTENUATION, 0.004f);
}

void depthFog(bool on, const Color& c, float start, float end) {
    if (!on) { glDisable(GL_FOG); return; }
    glEnable(GL_FOG);
    GLfloat col[4] = {c.r, c.g, c.b, 1};
    glFogi(GL_FOG_MODE, GL_LINEAR);                     // f = (end - z) / (end - start)
    glFogfv(GL_FOG_COLOR, col);
    glFogf(GL_FOG_START, start);
    glFogf(GL_FOG_END, end);
}

} // namespace vas::gfx
