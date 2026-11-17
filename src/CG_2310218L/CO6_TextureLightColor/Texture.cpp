#include "CG_2310218L/CO6_TextureLightColor/Texture.h"

#include <vector>

namespace vas::gfx {

GLuint makePcbTexture(int N) {
    // procedural PCB texture: grid, vias and copper traces              [CG CO6]
    std::vector<unsigned char> px(std::size_t(N) * N * 4);
    unsigned seed = 1234567;
    auto rnd = [&]() { seed = seed * 1103515245u + 12345u; return (seed >> 16) & 0x7FFF; };
    for (int y = 0; y < N; ++y)
        for (int x = 0; x < N; ++x) {
            unsigned char* p = &px[(std::size_t(y) * N + x) * 4];
            float g = ((x % 32 == 0) || (y % 32 == 0)) ? 1.f : 0.f;
            float noise = (rnd() % 100) / 100.f;
            p[0] = (unsigned char)(10 + 6 * noise + 10 * g);
            p[1] = (unsigned char)(20 + 8 * noise + 16 * g);
            p[2] = (unsigned char)(38 + 10 * noise + 24 * g);
            p[3] = 255;
        }
    auto plot = [&](int x, int y, int r, int g, int b) {
        if (x < 0 || y < 0 || x >= N || y >= N) return;
        unsigned char* p = &px[(std::size_t(y) * N + x) * 4];
        p[0] = (unsigned char)r; p[1] = (unsigned char)g; p[2] = (unsigned char)b;
    };
    for (int t = 0; t < 70; ++t) {   // copper traces: manhattan segments
        int x = rnd() % N, y = rnd() % N;
        int len = 30 + rnd() % 120;
        bool horiz = rnd() % 2;
        for (int k = 0; k < len; ++k) {
            for (int w = -1; w <= 1; ++w) {
                if (horiz) plot(x + k, y + w, 32, 64, 92);
                else plot(x + w, y + k, 32, 64, 92);
            }
        }
        // via at the end
        int ex = horiz ? x + len : x, ey = horiz ? y : y + len;
        for (int dy = -4; dy <= 4; ++dy)
            for (int dx = -4; dx <= 4; ++dx) {
                int d2 = dx * dx + dy * dy;
                if (d2 <= 16 && d2 >= 6) plot(ex + dx, ey + dy, 120, 150, 170);
                else if (d2 < 6) plot(ex + dx, ey + dy, 6, 10, 20);
            }
    }
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA, N, N, GL_RGBA, GL_UNSIGNED_BYTE, px.data());
    return tex;
}

} // namespace vas::gfx
