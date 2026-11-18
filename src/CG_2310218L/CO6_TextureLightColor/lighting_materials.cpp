// A directional light, a point light, a spot light and materials with ambient, diffuse, specular and emission.

#include <GL/gl.h>

// switches lighting on with smooth shading and unit-length normals
void enableLighting() {
    glEnable(GL_LIGHTING);
    glEnable(GL_NORMALIZE);
    glShadeModel(GL_SMOOTH);
}

// a directional light: w = 0 in the position means parallel rays, like the sun, coming from the direction (x, y, z)
void setDirectionalLight(int light, float x, float y, float z, const float diffuse[3], const float ambient[3]) {
    GLfloat position[4] = {x, y, z, 0.0f};
    GLfloat dif[4] = {diffuse[0], diffuse[1], diffuse[2], 1.0f};
    GLfloat amb[4] = {ambient[0], ambient[1], ambient[2], 1.0f};
    glEnable(light);
    glLightfv(light, GL_POSITION, position);
    glLightfv(light, GL_DIFFUSE, dif);
    glLightfv(light, GL_SPECULAR, dif);
    glLightfv(light, GL_AMBIENT, amb);
}

// a point light: w = 1 means the rays spread from a position and get weaker with distance;
// the strength is divided by constant + linear * d + quadratic * d * d
void setPointLight(int light, float x, float y, float z, const float diffuse[3], float quadraticAttenuation) {
    GLfloat position[4] = {x, y, z, 1.0f};
    GLfloat dif[4] = {diffuse[0], diffuse[1], diffuse[2], 1.0f};
    glEnable(light);
    glLightfv(light, GL_POSITION, position);
    glLightfv(light, GL_DIFFUSE, dif);
    glLightfv(light, GL_SPECULAR, dif);
    glLightf(light, GL_CONSTANT_ATTENUATION, 1.0f);
    glLightf(light, GL_LINEAR_ATTENUATION, 0.0f);
    glLightf(light, GL_QUADRATIC_ATTENUATION, quadraticAttenuation);
}

// a spot light: a point light that shines in a cone; outside the cone angle there is no light,
// and a higher exponent makes the beam concentrate in the middle
void setSpotLight(int light, const float position[3], const float direction[3], float coneDegrees, float exponent, const float diffuse[3]) {
    GLfloat pos[4] = {position[0], position[1], position[2], 1.0f};
    GLfloat dir[3] = {direction[0], direction[1], direction[2]};
    GLfloat dif[4] = {diffuse[0], diffuse[1], diffuse[2], 1.0f};
    glEnable(light);
    glLightfv(light, GL_POSITION, pos);
    glLightfv(light, GL_SPOT_DIRECTION, dir);
    glLightf(light, GL_SPOT_CUTOFF, coneDegrees);
    glLightf(light, GL_SPOT_EXPONENT, exponent);
    glLightfv(light, GL_DIFFUSE, dif);
    glLightfv(light, GL_SPECULAR, dif);
}

// a material: ambient (colour in shadow), diffuse (colour in light), specular (the highlight and its sharpness)
// and emission (light the surface gives off itself)
void setMaterial(const float color[3], const float specular[3], float shininess, const float emission[3]) {
    GLfloat amb[4] = {color[0] * 0.45f, color[1] * 0.45f, color[2] * 0.45f, 1.0f};
    GLfloat dif[4] = {color[0], color[1], color[2], 1.0f};
    GLfloat spe[4] = {specular[0], specular[1], specular[2], 1.0f};
    GLfloat emi[4] = {emission[0], emission[1], emission[2], 1.0f};
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, amb);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, dif);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, spe);
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emi);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, shininess);
}

// the diffuse brightness of a surface point: the dot product of the unit normal and the unit light direction, never below 0
float diffuseBrightness(const float normal[3], const float toLight[3]) {
    float d = normal[0] * toLight[0] + normal[1] * toLight[1] + normal[2] * toLight[2];
    if (d < 0.0f) {
        return 0.0f;
    }
    return d;
}
