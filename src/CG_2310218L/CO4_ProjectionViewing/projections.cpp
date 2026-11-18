// Perspective, orthographic, frustum, isometric and oblique projections.

#include <GL/gl.h>
#include <GL/glu.h>

#include <cmath>

const float PI_VALUE = 3.14159265f;

// perspective: a pyramid-shaped view volume; a point is divided by its depth, so far things look smaller
void setPerspective(float fovDegrees, float aspect, float nearPlane, float farPlane) {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(fovDegrees, aspect, nearPlane, farPlane);
    glMatrixMode(GL_MODELVIEW);
}

// orthographic (parallel): a box-shaped view volume; depth does not change the size
void setOrthographic(float halfHeight, float aspect, float nearPlane, float farPlane) {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-halfHeight * aspect, halfHeight * aspect, -halfHeight, halfHeight, nearPlane, farPlane);
    glMatrixMode(GL_MODELVIEW);
}

// frustum: the same pyramid as a perspective projection, given by the edges of the near plane;
// the edges are worked out from the field of view
void setFrustum(float fovDegrees, float aspect, float nearPlane, float farPlane) {
    float top = nearPlane * std::tan(fovDegrees * 0.5f * PI_VALUE / 180.0f);
    float right = top * aspect;
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-right, right, -top, top, nearPlane, farPlane);
    glMatrixMode(GL_MODELVIEW);
}

// isometric: a parallel projection seen along the diagonal of a cube, after turning 45 degrees about y and about 35.264 degrees about x,
// so the three axes are drawn with equal length and 120 degrees apart
void setIsometric(float halfHeight, float aspect) {
    setOrthographic(halfHeight, aspect, -1000.0f, 1000.0f);
    glLoadIdentity();
    glRotatef(35.264f, 1.0f, 0.0f, 0.0f);
    glRotatef(-45.0f, 0.0f, 1.0f, 0.0f);
}

// oblique (cabinet or cavalier): the front face stays undistorted and the depth axis is shear-drawn at an angle;
// the matrix adds depth times (scale cos a) to x and depth times (scale sin a) to y
void setOblique(float halfHeight, float aspect, float angleDegrees, float depthScale) {
    setOrthographic(halfHeight, aspect, -1000.0f, 1000.0f);
    float shear[16] = {1, 0, 0, 0,
                       0, 1, 0, 0,
                       depthScale * std::cos(angleDegrees * PI_VALUE / 180.0f), depthScale * std::sin(angleDegrees * PI_VALUE / 180.0f), 1, 0,
                       0, 0, 0, 1};
    glLoadIdentity();
    glMultMatrixf(shear);
}

// the size on screen of an object of a given height at a given distance: perspective divides by the distance
float perspectiveScreenHeight(float objectHeight, float distance, float fovDegrees, float screenHeight) {
    float viewHeight = 2.0f * distance * std::tan(fovDegrees * 0.5f * PI_VALUE / 180.0f);
    return objectHeight / viewHeight * screenHeight;
}
