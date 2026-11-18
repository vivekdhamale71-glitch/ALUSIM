// DDA and Bresenham lines, midpoint circle and midpoint ellipse drawn pixel by pixel.

#include <cmath>
#include <cstdlib>
#include <vector>

struct Pixel {
    int x;
    int y;
};

// DDA line: step one pixel along the longer direction and add a fixed fraction in the other direction
std::vector<Pixel> ddaLine(int x0, int y0, int x1, int y1) {
    std::vector<Pixel> pixels;
    int dx = x1 - x0;
    int dy = y1 - y0;
    int steps = std::abs(dx);
    if (std::abs(dy) > steps) {
        steps = std::abs(dy);
    }
    if (steps == 0) {
        Pixel only = {x0, y0};
        pixels.push_back(only);
        return pixels;
    }
    float xInc = static_cast<float>(dx) / static_cast<float>(steps);
    float yInc = static_cast<float>(dy) / static_cast<float>(steps);
    float x = static_cast<float>(x0);
    float y = static_cast<float>(y0);
    for (int i = 0; i <= steps; ++i) {
        Pixel p = {static_cast<int>(std::lround(x)), static_cast<int>(std::lround(y))};
        pixels.push_back(p);
        x += xInc;
        y += yInc;
    }
    return pixels;
}

// Bresenham line: only integers; the error term says when the line has moved far enough to step sideways
std::vector<Pixel> bresenhamLine(int x0, int y0, int x1, int y1) {
    std::vector<Pixel> pixels;
    int dx = std::abs(x1 - x0);
    int dy = -std::abs(y1 - y0);
    int stepX = 1;
    int stepY = 1;
    if (x0 > x1) {
        stepX = -1;
    }
    if (y0 > y1) {
        stepY = -1;
    }
    int error = dx + dy;
    while (true) {
        Pixel p = {x0, y0};
        pixels.push_back(p);
        if (x0 == x1 && y0 == y1) {
            break;
        }
        int doubled = 2 * error;
        if (doubled >= dy) {
            error += dy;
            x0 += stepX;
        }
        if (doubled <= dx) {
            error += dx;
            y0 += stepY;
        }
    }
    return pixels;
}

// one point of a circle gives eight, by the symmetry of the circle
static void plotEight(std::vector<Pixel>& pixels, int cx, int cy, int x, int y) {
    int xs[8] = {x, -x, x, -x, y, -y, y, -y};
    int ys[8] = {y, y, -y, -y, x, x, -x, -x};
    for (int i = 0; i < 8; ++i) {
        Pixel p = {cx + xs[i], cy + ys[i]};
        pixels.push_back(p);
    }
}

// midpoint circle: draw one eighth, and the decision value d tells whether the next pixel is east or south-east
std::vector<Pixel> midpointCircle(int cx, int cy, int radius) {
    std::vector<Pixel> pixels;
    int x = 0;
    int y = radius;
    int d = 1 - radius;
    while (x <= y) {
        plotEight(pixels, cx, cy, x, y);
        if (d < 0) {
            d += 2 * x + 3;
        } else {
            d += 2 * (x - y) + 5;
            --y;
        }
        ++x;
    }
    return pixels;
}

// one point of an ellipse gives four
static void plotFour(std::vector<Pixel>& pixels, int cx, int cy, int x, int y) {
    Pixel a = {cx + x, cy + y};
    Pixel b = {cx - x, cy + y};
    Pixel c = {cx + x, cy - y};
    Pixel d = {cx - x, cy - y};
    pixels.push_back(a);
    pixels.push_back(b);
    pixels.push_back(c);
    pixels.push_back(d);
}

// midpoint ellipse: region 1 (the curve is flatter than 45 degrees) steps in x, region 2 (steeper) steps in y
std::vector<Pixel> midpointEllipse(int cx, int cy, int rx, int ry) {
    std::vector<Pixel> pixels;
    long rx2 = static_cast<long>(rx) * rx;
    long ry2 = static_cast<long>(ry) * ry;
    int x = 0;
    int y = ry;
    long dx = 0;
    long dy = 2 * rx2 * y;
    double d1 = ry2 - rx2 * ry + 0.25 * rx2;
    while (dx < dy) {
        plotFour(pixels, cx, cy, x, y);
        ++x;
        dx += 2 * ry2;
        if (d1 < 0) {
            d1 += dx + ry2;
        } else {
            --y;
            dy -= 2 * rx2;
            d1 += dx - dy + ry2;
        }
    }
    double d2 = ry2 * (x + 0.5) * (x + 0.5) + rx2 * (y - 1.0) * (y - 1.0) - static_cast<double>(rx2) * ry2;
    while (y >= 0) {
        plotFour(pixels, cx, cy, x, y);
        --y;
        dy -= 2 * rx2;
        if (d2 > 0) {
            d2 += rx2 - dy;
        } else {
            ++x;
            dx += 2 * ry2;
            d2 += dx - dy + rx2;
        }
    }
    return pixels;
}
