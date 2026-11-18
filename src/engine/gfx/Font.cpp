#include "engine/gfx/Font.h"

#include "engine/gfx/Clipper.h"

#include <cmath>
#include <cstdio>
#include <map>
#include <memory>

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

namespace vas::gfx {

static std::vector<unsigned char> readFile(const std::string& path) {
    std::vector<unsigned char> data;
    FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) return data;
    std::fseek(f, 0, SEEK_END);
    long n = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    data.resize(std::size_t(n));
    if (n > 0 && std::fread(data.data(), 1, std::size_t(n), f) != std::size_t(n)) data.clear();
    std::fclose(f);
    return data;
}

unsigned nextCodepoint(const std::string& s, std::size_t& i) {
    unsigned char c = static_cast<unsigned char>(s[i++]);
    if (c < 0x80) return c;
    int extra = (c >= 0xF0) ? 3 : (c >= 0xE0) ? 2 : (c >= 0xC0) ? 1 : 0;
    unsigned cp = c & (0x3F >> extra);
    for (int k = 0; k < extra && i < s.size(); ++k) cp = (cp << 6) | (static_cast<unsigned char>(s[i++]) & 0x3F);
    return cp;
}

// symbols borrowed from DejaVu Sans (arrows, check marks, math)
static const int kSymbols[] = {0x2190, 0x2191, 0x2192, 0x2193, 0x2022, 0x00D7, 0x2260, 0x2264, 0x2265, 0x2713,
                               0x2717, 0x2026, 0x00B7, 0x21BB, 0x25B6, 0x25C0, 0x25CF, 0x2194, 0x21D2, 0x2318,
                               0x23CE, 0x2502, 0x25B2, 0x25BC, 0x2014, 0x2013, 0x00B2, 0x2080, 0x2081, 0x03A3};

bool Font::load(const std::string& path, const std::string& symbolPath, float px) {
    std::vector<unsigned char> data = readFile(path);
    if (data.empty()) {
        std::fprintf(stderr, "Could not read font %s\n", path.c_str());
        return false;
    }
    std::vector<unsigned char> sym = readFile(symbolPath);
    px_ = px;
    stbtt_fontinfo info;
    stbtt_InitFont(&info, data.data(), stbtt_GetFontOffsetForIndex(data.data(), 0));
    float scale = stbtt_ScaleForPixelHeight(&info, px);
    int a, d, g;
    stbtt_GetFontVMetrics(&info, &a, &d, &g);
    ascent_ = a * scale;
    descent_ = d * scale;
    lineGap_ = g * scale;

    atlasW_ = px <= 20 ? 512 : 1024;
    atlasH_ = px <= 14 ? 256 : 512;
    if (px > 40) atlasH_ = 1024;
    std::vector<unsigned char> pixels(std::size_t(atlasW_) * atlasH_);
    stbtt_pack_context pc;
    stbtt_PackBegin(&pc, pixels.data(), atlasW_, atlasH_, 0, 1, nullptr);
    stbtt_PackSetOversampling(&pc, px < 30 ? 2 : 1, px < 30 ? 2 : 1);

    std::vector<stbtt_packedchar> asc(95);
    stbtt_PackFontRange(&pc, data.data(), 0, px, 32, 95, asc.data());

    const int nsym = int(sizeof kSymbols / sizeof kSymbols[0]);
    std::vector<stbtt_packedchar> sp(nsym);
    if (!sym.empty()) {
        stbtt_pack_range r{};
        r.font_size = px;
        r.array_of_unicode_codepoints = const_cast<int*>(kSymbols);
        r.num_chars = nsym;
        r.chardata_for_range = sp.data();
        stbtt_PackFontRanges(&pc, sym.data(), 0, &r, 1);
    }
    stbtt_PackEnd(&pc);

    auto conv = [](const stbtt_packedchar& p) {
        return Glyph{p.x0, p.y0, p.x1, p.y1, p.xoff, p.yoff, p.xadvance, p.xoff2, p.yoff2};
    };
    ascii_.clear();
    for (auto& p : asc) ascii_.push_back(conv(p));
    extra_.clear();
    if (!sym.empty())
        for (int i = 0; i < nsym; ++i) extra_.push_back({unsigned(kSymbols[i]), conv(sp[i])});

    glGenTextures(1, &tex_);
    glBindTexture(GL_TEXTURE_2D, tex_);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, atlasW_, atlasH_, 0, GL_ALPHA, GL_UNSIGNED_BYTE, pixels.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    return true;
}

const Font::Glyph* Font::glyph(unsigned cp) const {
    if (cp >= 32 && cp < 127 && !ascii_.empty()) return &ascii_[cp - 32];
    for (const auto& e : extra_) if (e.first == cp) return &e.second;
    return ascii_.empty() ? nullptr : &ascii_['?' - 32];
}

float Font::width(const std::string& s) const {
    float w = 0;
    for (std::size_t i = 0; i < s.size();) {
        const Glyph* g = glyph(nextCodepoint(s, i));
        if (g) w += g->xadvance;
    }
    return w;
}

float Font::draw(float x, float yTop, const std::string& s, const Color& c) const {
    if (!tex_) return 0;
    float baseline = std::round(yTop + ascent_);
    float x0 = x;
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex_);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    float iw = 1.0f / atlasW_, ih = 1.0f / atlasH_;
    for (std::size_t i = 0; i < s.size();) {
        const Glyph* g = glyph(nextCodepoint(s, i));
        if (!g) continue;
        // one textured quad per glyph; (u, v) selects the glyph in the atlas.
        // It goes through the software clipper like every other shape (CG CO5),
        // which interpolates (u, v) where a glyph is cut.
        float qx0 = x + g->xoff, qy0 = baseline + g->yoff, qx1 = x + g->xoff2, qy1 = baseline + g->yoff2;
        float s0 = g->x0 * iw, t0 = g->y0 * ih, s1 = g->x1 * iw, t1 = g->y1 * ih;
        clipper::polygon({{qx0, qy0, c.r, c.g, c.b, c.a, s0, t0}, {qx1, qy0, c.r, c.g, c.b, c.a, s1, t0},
                          {qx1, qy1, c.r, c.g, c.b, c.a, s1, t1}, {qx0, qy1, c.r, c.g, c.b, c.a, s0, t1}},
                         true);
        x += g->xadvance;
    }
    glDisable(GL_TEXTURE_2D);
    return x - x0;
}

// ------------------------------------------------------------------ cache
static std::string& assetDirRef() {
    static std::string d = "assets";
    return d;
}
void Fonts::setAssetDir(const std::string& dir) { assetDirRef() = dir; }
const std::string& Fonts::assetDir() { return assetDirRef(); }

Font& Fonts::get(Face face, int px) {
    static std::map<std::pair<int, int>, std::unique_ptr<Font>> cache;
    auto key = std::make_pair(int(face), px);
    auto it = cache.find(key);
    if (it != cache.end()) return *it->second;
    const char* file = "Lato-Regular.ttf";
    switch (face) {
        case Face::Regular: file = "Lato-Regular.ttf"; break;
        case Face::Semibold: file = "Lato-Semibold.ttf"; break;
        case Face::Bold: file = "Lato-Bold.ttf"; break;
        case Face::Mono: file = "NotoSansMono-Regular.ttf"; break;
        case Face::MonoBold: file = "NotoSansMono-Bold.ttf"; break;
    }
    auto f = std::make_unique<Font>();
    std::string dir = assetDir() + "/fonts/";
    f->load(dir + file, dir + "DejaVuSans.ttf", float(px));
    Font& ref = *f;
    cache[key] = std::move(f);
    return ref;
}

} // namespace vas::gfx
