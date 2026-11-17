// =====================================================================
//  Procedural texture image for the chip's circuit board    [CG CO6]
// =====================================================================
#pragma once
#include "CG_2310218L/CO1_Primitives/GL.h"

namespace vas::gfx {

// Generates an N x N RGBA image (grid, noise, copper traces, vias),
// uploads it with gluBuild2DMipmaps and returns the texture id.
// Filtering: trilinear (GL_LINEAR_MIPMAP_LINEAR), wrapping: GL_REPEAT.
GLuint makePcbTexture(int N = 512);

} // namespace vas::gfx
