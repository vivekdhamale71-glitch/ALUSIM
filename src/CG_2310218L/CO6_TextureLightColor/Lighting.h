// =====================================================================
//  Light, material and fog for the 3D chip                  [CG CO6]
// =====================================================================
#pragma once
#include "CG_2310218L/CO2_Transformations/Math.h"

namespace vas::gfx {

// Phong material: ambient + diffuse from the colour, white-ish specular,
// emission makes the active unit glow
void material(const Color& c, const Color& emission, float shininess = 48);

// GL_LIGHT0: white directional key light
// GL_LIGHT1: point light tinted with the colour of the current pipeline
//            phase, with quadratic attenuation
void chipLights(const Color& phase);

// linear depth fog between start and end (far units fade into the background)
void depthFog(bool on, const Color& color, float start, float end);

} // namespace vas::gfx
