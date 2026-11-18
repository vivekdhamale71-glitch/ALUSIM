// Translate, rotate, scale, reflect and shear with a 3x3 matrix.

#include <cmath>

struct Point2 {
    float x;
    float y;
};

// a 2D point is written as (x, y, 1) so that translation can also be a matrix; m is stored row by row
struct Matrix3 {
    float m[9];
};

// the matrix that changes nothing
Matrix3 identity() {
    Matrix3 r = {{1, 0, 0, 0, 1, 0, 0, 0, 1}};
    return r;
}

// moves every point by (tx, ty)
Matrix3 translation(float tx, float ty) {
    Matrix3 r = identity();
    r.m[2] = tx;
    r.m[5] = ty;
    return r;
}

// turns every point by an angle (in radians) around the origin
Matrix3 rotation(float radians) {
    Matrix3 r = identity();
    r.m[0] = std::cos(radians);
    r.m[1] = -std::sin(radians);
    r.m[3] = std::sin(radians);
    r.m[4] = std::cos(radians);
    return r;
}

// stretches x by sx and y by sy, away from the origin
Matrix3 scaling(float sx, float sy) {
    Matrix3 r = identity();
    r.m[0] = sx;
    r.m[4] = sy;
    return r;
}

// mirrors across the x axis (y changes sign)
Matrix3 reflectionX() {
    return scaling(1.0f, -1.0f);
}

// mirrors across the y axis (x changes sign)
Matrix3 reflectionY() {
    return scaling(-1.0f, 1.0f);
}

// slants the shape: x moves by shx times y, y moves by shy times x
Matrix3 shear(float shx, float shy) {
    Matrix3 r = identity();
    r.m[1] = shx;
    r.m[3] = shy;
    return r;
}

// the product a * b: row i of a times column j of b; the result applies b first and then a
Matrix3 multiply(const Matrix3& a, const Matrix3& b) {
    Matrix3 r;
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            r.m[i * 3 + j] = a.m[i * 3] * b.m[j] + a.m[i * 3 + 1] * b.m[3 + j] + a.m[i * 3 + 2] * b.m[6 + j];
        }
    }
    return r;
}

// sends one point through the matrix
Point2 apply(const Matrix3& a, Point2 p) {
    Point2 r;
    r.x = a.m[0] * p.x + a.m[1] * p.y + a.m[2];
    r.y = a.m[3] * p.x + a.m[4] * p.y + a.m[5];
    return r;
}

// rotation about any point: move the pivot to the origin, rotate, then move it back
Matrix3 rotationAbout(Point2 pivot, float radians) {
    Matrix3 toOrigin = translation(-pivot.x, -pivot.y);
    Matrix3 back = translation(pivot.x, pivot.y);
    return multiply(back, multiply(rotation(radians), toOrigin));
}

// scaling about any point, the same three steps as rotation about a point
Matrix3 scalingAbout(Point2 pivot, float sx, float sy) {
    Matrix3 toOrigin = translation(-pivot.x, -pivot.y);
    Matrix3 back = translation(pivot.x, pivot.y);
    return multiply(back, multiply(scaling(sx, sy), toOrigin));
}
