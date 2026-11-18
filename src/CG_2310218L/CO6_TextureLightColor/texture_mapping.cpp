// A generated texture with mipmaps, filtering and wrapping, and a glyph texture for text.

#include <GL/gl.h>
#include <GL/glu.h>

#include <vector>

// a repeatable pseudo-random number, so the generated texture is the same every time
static unsigned randomState = 1234567;

static unsigned nextRandom() {
    randomState = randomState * 1103515245u + 12345u;
    return (randomState >> 16) & 0x7FFF;
}

// writes one colour into a pixel of an N x N RGBA picture; ignored outside the picture
static void setPixel(std::vector<unsigned char>& pixels, int n, int x, int y, int r, int g, int b) {
    if (x < 0 || y < 0 || x >= n || y >= n) {
        return;
    }
    unsigned char* p = &pixels[(static_cast<std::size_t>(y) * n + x) * 4];
    p[0] = static_cast<unsigned char>(r);
    p[1] = static_cast<unsigned char>(g);
    p[2] = static_cast<unsigned char>(b);
    p[3] = 255;
}

// the picture itself: a dark base with noise, a line every 32 pixels, and short copper traces that end in a round via
std::vector<unsigned char> makeBoardPixels(int n) {
    std::vector<unsigned char> pixels(static_cast<std::size_t>(n) * n * 4);
    randomState = 1234567;
    for (int y = 0; y < n; ++y) {
        for (int x = 0; x < n; ++x) {
            int grid = 0;
            if (x % 32 == 0 || y % 32 == 0) {
                grid = 1;
            }
            int noise = static_cast<int>(nextRandom() % 100);
            setPixel(pixels, n, x, y, 10 + 6 * noise / 100 + 10 * grid, 20 + 8 * noise / 100 + 16 * grid, 38 + 10 * noise / 100 + 24 * grid);
        }
    }
    for (int trace = 0; trace < 70; ++trace) {
        int x = static_cast<int>(nextRandom() % n);
        int y = static_cast<int>(nextRandom() % n);
        int length = 30 + static_cast<int>(nextRandom() % 120);
        bool horizontal = nextRandom() % 2 == 0;
        for (int k = 0; k < length; ++k) {
            for (int w = -1; w <= 1; ++w) {
                if (horizontal) {
                    setPixel(pixels, n, x + k, y + w, 32, 64, 92);
                } else {
                    setPixel(pixels, n, x + w, y + k, 32, 64, 92);
                }
            }
        }
    }
    return pixels;
}

// uploads the picture as a texture: the filters decide how it is sampled when it is smaller or bigger on screen,
// the wrap mode decides what happens outside 0..1, and the mipmaps are pre-shrunk copies for distant surfaces
unsigned uploadTexture(const std::vector<unsigned char>& pixels, int n) {
    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA, n, n, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    return texture;
}

// draws a quad with the texture: each corner gets a texture coordinate (u, v); repeat says how many times the picture tiles
void drawTexturedQuad(unsigned texture, float x, float y, float w, float h, float repeat) {
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texture);
    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f);
    glVertex2f(x, y);
    glTexCoord2f(repeat, 0.0f);
    glVertex2f(x + w, y);
    glTexCoord2f(repeat, repeat);
    glVertex2f(x + w, y + h);
    glTexCoord2f(0.0f, repeat);
    glVertex2f(x, y + h);
    glEnd();
    glDisable(GL_TEXTURE_2D);
}

// a glyph atlas: one single-channel texture that holds the letters; the value is the opacity of the letter,
// and clamping stops a letter from picking up its neighbour at the edge
unsigned uploadGlyphAtlas(const std::vector<unsigned char>& alpha, int width, int height) {
    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, width, height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, alpha.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    return texture;
}

// draws one letter: a quad whose texture coordinates pick the letter's rectangle out of the atlas,
// coloured by the current colour (the atlas only gives the shape)
void drawGlyph(unsigned atlas, float x, float y, float w, float h, float u0, float v0, float u1, float v1) {
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, atlas);
    glBegin(GL_QUADS);
    glTexCoord2f(u0, v0);
    glVertex2f(x, y);
    glTexCoord2f(u1, v0);
    glVertex2f(x + w, y);
    glTexCoord2f(u1, v1);
    glVertex2f(x + w, y + h);
    glTexCoord2f(u0, v1);
    glVertex2f(x, y + h);
    glEnd();
    glDisable(GL_TEXTURE_2D);
}
