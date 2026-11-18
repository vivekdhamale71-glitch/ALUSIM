// Linear fog, the HSV to RGB colour model and alpha blending.

#include <GL/gl.h>

#include <cmath>

// linear fog: the colour fades to the fog colour as the distance from the eye goes from start to end
void enableLinearFog(const float color[3], float start, float end) {
    GLfloat c[4] = {color[0], color[1], color[2], 1.0f};
    glEnable(GL_FOG);
    glFogi(GL_FOG_MODE, GL_LINEAR);
    glFogfv(GL_FOG_COLOR, c);
    glFogf(GL_FOG_START, start);
    glFogf(GL_FOG_END, end);
}

// switches the fog off
void disableFog() {
    glDisable(GL_FOG);
}

// the share of the object's own colour left at a distance: f = (end - z) / (end - start), kept between 0 and 1
float fogFactor(float distance, float start, float end) {
    float f = (end - distance) / (end - start);
    if (f < 0.0f) {
        return 0.0f;
    }
    if (f > 1.0f) {
        return 1.0f;
    }
    return f;
}

// the colour after fog: f of the object's colour plus (1 - f) of the fog colour
void applyFog(const float object[3], const float fog[3], float f, float out[3]) {
    for (int i = 0; i < 3; ++i) {
        out[i] = object[i] * f + fog[i] * (1.0f - f);
    }
}

// HSV to RGB: hue (0..360) picks the colour on the colour wheel, saturation how pure it is, value how bright
void hsvToRgb(float hue, float saturation, float value, float rgb[3]) {
    float c = value * saturation;
    float sector = hue / 60.0f;
    float x = c * (1.0f - std::fabs(std::fmod(sector, 2.0f) - 1.0f));
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    if (sector < 1.0f) {
        r = c;
        g = x;
    } else if (sector < 2.0f) {
        r = x;
        g = c;
    } else if (sector < 3.0f) {
        g = c;
        b = x;
    } else if (sector < 4.0f) {
        g = x;
        b = c;
    } else if (sector < 5.0f) {
        r = x;
        b = c;
    } else {
        r = c;
        b = x;
    }
    float m = value - c;
    rgb[0] = r + m;
    rgb[1] = g + m;
    rgb[2] = b + m;
}

// the colour in between two colours: a + (b - a) * t
void mixColors(const float a[3], const float b[3], float t, float out[3]) {
    for (int i = 0; i < 3; ++i) {
        out[i] = a[i] + (b[i] - a[i]) * t;
    }
}

// normal blending: result = new * alpha + old * (1 - alpha), so a colour with alpha below 1 is see-through
void enableAlphaBlending() {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

// additive blending: result = new * alpha + old, so overlapping lights add up (used for glows)
void enableAdditiveBlending() {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
}

// the blend of one colour channel, written out as the formula
float blendChannel(float source, float destination, float alpha) {
    return source * alpha + destination * (1.0f - alpha);
}
