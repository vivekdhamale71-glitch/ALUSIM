// =====================================================================
//  Orbit camera with an animated perspective -> parallel switch [CG CO4]
//  Used by the 3D chip and its "3D <-> 2D" button.
// =====================================================================
#pragma once

namespace vas::gfx {

// Sets GL_PROJECTION and GL_MODELVIEW.
//   yaw, pitch, dist  orbit position around the target (degrees, units)
//   flat  0 = perspective, 1 = orthographic top view. In between, a dolly
//         zoom narrows the field of view while the eye backs away, so the
//         subject keeps its size while the projection lines become parallel.
void orbitCamera(float aspect, float yaw, float pitch, float dist, float flat, float tx, float ty, float tz);

} // namespace vas::gfx
