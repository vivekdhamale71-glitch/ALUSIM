// =====================================================================
//  2D geometric transformations with homogeneous 3x3 matrices  [CG CO2]
//  Used for the datapath pan / zoom: the view matrix is
//      M = T(centre + pan) * S(zoom) * T(-world centre)
//  It maps every wire to screen space before clipping (CO5), and its
//  inverse maps the mouse back to world space so zoom stays centred on
//  the cursor (app/DatapathView.cpp).
// =====================================================================
#include "engine/gfx/Math.h"

namespace vas::gfx {

Mat3 Mat3::translate(float tx, float ty) {
    Mat3 r;
    r.m[2] = tx;
    r.m[5] = ty;
    return r;
}

Mat3 Mat3::scale(float sx, float sy) {
    Mat3 r;
    r.m[0] = sx;
    r.m[4] = sy;
    return r;
}

Mat3 Mat3::rotate(float rad) {
    Mat3 r;
    float c = std::cos(rad), s = std::sin(rad);
    r.m[0] = c; r.m[1] = -s;
    r.m[3] = s; r.m[4] = c;
    return r;
}

// row i of this times column j of o
Mat3 Mat3::operator*(const Mat3& o) const {
    Mat3 r;
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            r.m[i * 3 + j] = m[i * 3] * o.m[j] + m[i * 3 + 1] * o.m[3 + j] + m[i * 3 + 2] * o.m[6 + j];
    return r;
}

Vec2 Mat3::apply(Vec2 p) const { return {m[0] * p.x + m[1] * p.y + m[2], m[3] * p.x + m[4] * p.y + m[5]}; }

float Mat3::scaleX() const { return std::sqrt(m[0] * m[0] + m[3] * m[3]); }

// inverse of [A t; 0 1] is [A^-1  -A^-1 t; 0 1]
Mat3 Mat3::inverseAffine() const {
    float det = m[0] * m[4] - m[1] * m[3];
    Mat3 r;
    r.m[0] = m[4] / det;  r.m[1] = -m[1] / det;
    r.m[3] = -m[3] / det; r.m[4] = m[0] / det;
    r.m[2] = -(r.m[0] * m[2] + r.m[1] * m[5]);
    r.m[5] = -(r.m[3] * m[2] + r.m[4] * m[5]);
    return r;
}

} // namespace vas::gfx
