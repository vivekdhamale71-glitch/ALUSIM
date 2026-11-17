// =====================================================================
//  TrueType text rendering                                [CG CO6]
//  Glyphs are rasterised with stb_truetype into an alpha texture atlas;
//  each character is drawn as a textured quad (texture mapping), which
//  also lets the 3D chip view print labels onto the chip surfaces.
// =====================================================================
#pragma once
#include <string>
#include <vector>

#include "CG_2310218L/CO1_Primitives/GL.h"
#include "CG_2310218L/CO2_Transformations/Math.h"

namespace vas::gfx {

enum class Face { Regular, Semibold, Bold, Mono, MonoBold };

class Font {
public:
    bool load(const std::string& path, const std::string& symbolPath, float px);
    // draws with the top of the line box at yTop; returns the advance width
    float draw(float x, float yTop, const std::string& utf8, const Color& c) const;
    float width(const std::string& utf8) const;
    float ascent() const { return ascent_; }
    float lineHeight() const { return ascent_ - descent_ + lineGap_; }
    float height() const { return ascent_ - descent_; }
    float px() const { return px_; }

private:
    struct Glyph {
        unsigned short x0, y0, x1, y1;
        float xoff, yoff, xadvance, xoff2, yoff2;
    };
    const Glyph* glyph(unsigned cp) const;

    GLuint tex_ = 0;
    int atlasW_ = 0, atlasH_ = 0;
    float px_ = 0, ascent_ = 0, descent_ = 0, lineGap_ = 0;
    std::vector<Glyph> ascii_;                    // 32..126
    std::vector<std::pair<unsigned, Glyph>> extra_;
};

class Fonts {
public:
    static void setAssetDir(const std::string& dir);
    static const std::string& assetDir();
    static Font& get(Face face, int px);
};

// UTF-8 helpers
unsigned nextCodepoint(const std::string& s, std::size_t& i);

} // namespace vas::gfx
