// =====================================================================
//  Scene3D - small helper for 3D panels inside the 2D interface
//                                                  [CG CO2, CO3, CO4, CO6]
//  Sets up a viewport, a perspective camera (gluPerspective + gluLookAt),
//  two lights and materials, draws lit boxes, and projects 3D points
//  back to the screen (gluProject) so labels can be drawn crisply in 2D.
//  Used by the 3D sorting bars, the 3D bit-register cubes and the 3D
//  pipeline conveyor.
// =====================================================================
#pragma once
#include "engine/gfx/Math.h"

namespace vas::gfx::s3d {

struct Camera {
    float yaw = -25, pitch = 25, dist = 12;
    float tx = 0, ty = 0, tz = 0;
    float fov = 32;
};

// begins a 3D pass clipped to `vp` (screen pixels, top-left origin)
void begin(const Rect& vp, const Camera& cam, bool lightTheme);
// ends the pass and restores the 2D state
void end();

void material(const Color& c, float emission = 0.f, float shininess = 50.f);
void box(float x0, float y0, float z0, float x1, float y1, float z1);
// a box rotated about the X axis around its centre (used for flipping bits)
void boxRotX(float cx, float cy, float cz, float sx, float sy, float sz, float angleDeg);
// translucent flat quad on the floor (unlit) for halos / shadows
void floorQuad(float x0, float z0, float x1, float z1, float y, const Color& c);

// project a 3D point to screen pixels (valid between begin() and end())
bool project(float x, float y, float z, Vec2& out);

} // namespace vas::gfx::s3d
