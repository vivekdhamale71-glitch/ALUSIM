// An orbit camera with gluLookAt and a dolly zoom from perspective to a parallel view.

#include <GL/gl.h>
#include <GL/glu.h>

#include <algorithm>
#include <cmath>

const float PI_ANGLE = 3.14159265f;

// the camera stands on a sphere around a target point: yaw turns it left and right, pitch up and down
void orbitPosition(float yaw, float pitch, float distance, const float target[3], float eye[3]) {
    float yawRad = yaw * PI_ANGLE / 180.0f;
    float pitchRad = pitch * PI_ANGLE / 180.0f;
    eye[0] = target[0] + distance * std::cos(pitchRad) * std::sin(yawRad);
    eye[1] = target[1] + distance * std::sin(pitchRad);
    eye[2] = target[2] + distance * std::cos(pitchRad) * std::cos(yawRad);
}

// the viewing matrix: eye, the point looked at, and the up direction
void applyLookAt(const float eye[3], const float target[3]) {
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(eye[0], eye[1], eye[2], target[0], target[1], target[2], 0.0f, 1.0f, 0.0f);
}

// dragging the mouse: horizontal movement changes yaw, vertical movement changes pitch (kept below the poles)
void dragOrbit(float& yaw, float& pitch, float dx, float dy) {
    yaw -= dx * 0.4f;
    pitch += dy * 0.4f;
    pitch = std::max(-89.0f, std::min(89.0f, pitch));
}

// the wheel changes the distance by a fixed factor, within limits
float zoomDistance(float distance, float wheel, float minDistance, float maxDistance) {
    float factor = 1.0f - 0.1f * wheel;
    float result = distance * factor;
    return std::max(minDistance, std::min(maxDistance, result));
}

// dolly zoom: f = 0 is perspective with a 38 degree field of view; f = 1 is a parallel view.
// The field of view narrows and the eye moves back so that the height of the view at the target, d * tan(fov / 2), never changes.
void dollyZoomCamera(float aspect, float yaw, float pitch, float distance, float f, const float target[3]) {
    float fov = 38.0f + (2.5f - 38.0f) * f;
    float baseHalf = std::tan(19.0f * PI_ANGLE / 180.0f);
    float d = distance * baseHalf / std::tan(fov * 0.5f * PI_ANGLE / 180.0f);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    if (f > 0.985f) {
        float half = distance * baseHalf;
        glOrtho(-half * aspect, half * aspect, -half, half, -1000.0, 1000.0);
        d = distance;
    } else {
        gluPerspective(fov, aspect, std::max(0.3f, d - distance * 1.6f), d + distance * 4.0f);
    }
    float eye[3];
    orbitPosition(yaw, pitch, d, target, eye);
    applyLookAt(eye, target);
}

// turning between two yaw angles by the shortest way round (never more than half a turn)
float shortestTurn(float from, float to) {
    return std::fmod(to - from + 540.0f, 360.0f) - 180.0f;
}

// smooth start and end for the animation of f
float easeInOut(float t) {
    t = std::max(0.0f, std::min(1.0f, t));
    if (t < 0.5f) {
        return 4.0f * t * t * t;
    }
    return 1.0f - std::pow(-2.0f * t + 2.0f, 3.0f) / 2.0f;
}
