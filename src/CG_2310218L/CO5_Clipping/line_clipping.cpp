// Cohen-Sutherland and Liang-Barsky line clipping.

struct Point2 {
    float x;
    float y;
};

// the clipping window: left, top, right and bottom edges (the y axis grows downwards, as on the screen)
struct Window {
    float left;
    float top;
    float right;
    float bottom;
};

// one bit for each side of the window
const int INSIDE_CODE = 0;
const int LEFT_CODE = 1;
const int RIGHT_CODE = 2;
const int TOP_CODE = 4;
const int BOTTOM_CODE = 8;

// the 4-bit outcode of a point: which sides of the window it is beyond (0 means inside)
int outCode(Point2 p, const Window& w) {
    int code = INSIDE_CODE;
    if (p.x < w.left) {
        code |= LEFT_CODE;
    } else if (p.x > w.right) {
        code |= RIGHT_CODE;
    }
    if (p.y < w.top) {
        code |= TOP_CODE;
    } else if (p.y > w.bottom) {
        code |= BOTTOM_CODE;
    }
    return code;
}

// Cohen-Sutherland: accept if both codes are 0, reject if the codes share a bit,
// otherwise move an outside end point to the window edge and test again; returns false when the line is rejected
bool cohenSutherland(Point2& a, Point2& b, const Window& w) {
    int codeA = outCode(a, w);
    int codeB = outCode(b, w);
    while (true) {
        if ((codeA | codeB) == 0) {
            return true;
        }
        if ((codeA & codeB) != 0) {
            return false;
        }
        int outside = codeA;
        if (outside == 0) {
            outside = codeB;
        }
        Point2 p;
        if ((outside & TOP_CODE) != 0) {
            p.x = a.x + (b.x - a.x) * (w.top - a.y) / (b.y - a.y);
            p.y = w.top;
        } else if ((outside & BOTTOM_CODE) != 0) {
            p.x = a.x + (b.x - a.x) * (w.bottom - a.y) / (b.y - a.y);
            p.y = w.bottom;
        } else if ((outside & RIGHT_CODE) != 0) {
            p.y = a.y + (b.y - a.y) * (w.right - a.x) / (b.x - a.x);
            p.x = w.right;
        } else {
            p.y = a.y + (b.y - a.y) * (w.left - a.x) / (b.x - a.x);
            p.x = w.left;
        }
        if (outside == codeA) {
            a = p;
            codeA = outCode(a, w);
        } else {
            b = p;
            codeB = outCode(b, w);
        }
    }
}

// Liang-Barsky: the visible part of the line is the range t0..t1 of a + t(b - a) that satisfies p*t <= q for all four edges;
// returns false when the range is empty
bool liangBarskyRange(Point2 a, Point2 b, const Window& w, float& t0, float& t1) {
    float dx = b.x - a.x;
    float dy = b.y - a.y;
    float p[4] = {-dx, dx, -dy, dy};
    float q[4] = {a.x - w.left, w.right - a.x, a.y - w.top, w.bottom - a.y};
    t0 = 0.0f;
    t1 = 1.0f;
    for (int i = 0; i < 4; ++i) {
        if (p[i] == 0.0f) {
            if (q[i] < 0.0f) {
                return false;
            }
            continue;
        }
        float t = q[i] / p[i];
        if (p[i] < 0.0f) {
            if (t > t1) {
                return false;
            }
            if (t > t0) {
                t0 = t;
            }
        } else {
            if (t < t0) {
                return false;
            }
            if (t < t1) {
                t1 = t;
            }
        }
    }
    return true;
}

// Liang-Barsky clipping of the end points themselves
bool liangBarsky(Point2& a, Point2& b, const Window& w) {
    float t0;
    float t1;
    if (!liangBarskyRange(a, b, w, t0, t1)) {
        return false;
    }
    Point2 newA = {a.x + (b.x - a.x) * t0, a.y + (b.y - a.y) * t0};
    Point2 newB = {a.x + (b.x - a.x) * t1, a.y + (b.y - a.y) * t1};
    a = newA;
    b = newB;
    return true;
}
