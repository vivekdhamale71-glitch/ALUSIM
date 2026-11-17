#include "CG_2310218L/CO4_ProjectionViewing/Camera.h"

#include <algorithm>
#include <cmath>

#include "CG_2310218L/CO1_Primitives/GL.h"

namespace vas::gfx {

void orbitCamera(float aspect, float yaw, float pitch, float dist, float f, float tx, float ty, float tz) {
    // dolly zoom: fov 38 deg -> 2.5 deg, distance grows so the view height
    // at the target, d * tan(fov / 2), stays the same
    float fov = 38 + (2.5f - 38) * f;
    float base = std::tan(19 * float(M_PI) / 180);
    float d = dist * base / std::tan(fov * 0.5f * float(M_PI) / 180);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    if (f > 0.985f) {                                   // parallel projection
        float s = dist * base;
        glOrtho(-s * aspect, s * aspect, -s, s, -1000, 1000);
        d = dist;
    } else {                                            // perspective projection
        gluPerspective(fov, aspect, std::max(0.3f, d - dist * 1.6f), d + dist * 4);
    }

    // viewing: eye on a sphere around the target, looking at it, y up
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    float yr = yaw * float(M_PI) / 180, pr = pitch * float(M_PI) / 180;
    gluLookAt(tx + d * std::cos(pr) * std::sin(yr), ty + d * std::sin(pr), tz + d * std::cos(pr) * std::cos(yr),
              tx, ty, tz, 0, 1, 0);
}

} // namespace vas::gfx
