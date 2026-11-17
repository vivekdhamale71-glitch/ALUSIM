// Colour palette with a dark ("midnight") and a light ("daylight") theme.
// Every view reads these variables, so switching is a single call to apply().
#pragma once
#include "COA_2310221L/CO4_ControlUnitPipeline/ControlUnit.h"
#include "CG_2310218L/CO1_Primitives/Draw.h"
#include "CG_2310218L/CO2_Transformations/Math.h"

namespace vas::ui::theme {

using gfx::Color;

inline bool light = false;

inline Color bg0, bg1, panel, panel2, panel3, border, borderHi, text, textDim, textMute;
inline Color cyan, blue, violet, pink, amber, green, red, orange, teal;
inline Color surface;    // deep inset areas: editor, canvases
inline Color overlay;    // floating labels / tooltips / toasts
inline Color onAccent;   // text drawn on a filled accent colour
inline Color ident;      // identifiers, register names
inline Color unitTop, unitBottom, unitBorder;   // datapath blocks
inline Color wire;       // idle datapath wires
inline float shadowAlpha = 1.0f;

inline void apply(bool lightMode) {
    light = lightMode;
    gfx::draw::setLightMode(lightMode);
    if (!lightMode) {
        bg0 = Color::hex(0x060A14); bg1 = Color::hex(0x0B1222);
        panel = Color::hex(0x101A2E); panel2 = Color::hex(0x15213A); panel3 = Color::hex(0x1B2946);
        border = Color::hex(0x22304C); borderHi = Color::hex(0x334670);
        text = Color::hex(0xE6EDF7); textDim = Color::hex(0xA3B1CC); textMute = Color::hex(0x66758F);
        cyan = Color::hex(0x38BDF8); blue = Color::hex(0x60A5FA); violet = Color::hex(0xA78BFA);
        pink = Color::hex(0xF472B6); amber = Color::hex(0xFBBF24); green = Color::hex(0x34D399);
        red = Color::hex(0xF87171); orange = Color::hex(0xFB923C); teal = Color::hex(0x2DD4BF);
        surface = Color::hex(0x0A1324); overlay = Color::hex(0x0B1222);
        onAccent = Color::hex(0x06101E); ident = Color::hex(0x7DD3FC);
        unitTop = Color::hex(0x1A2846); unitBottom = Color::hex(0x121C33); unitBorder = Color::hex(0x34507F);
        wire = Color::hex(0x2A3A5E);
        shadowAlpha = 1.0f;
    } else {
        bg0 = Color::hex(0xEEF2F8); bg1 = Color::hex(0xE4EAF3);
        panel = Color::hex(0xFFFFFF); panel2 = Color::hex(0xF5F8FC); panel3 = Color::hex(0xE9EFF8);
        border = Color::hex(0xD6DFEC); borderHi = Color::hex(0xB6C4DA);
        text = Color::hex(0x0F172A); textDim = Color::hex(0x475569); textMute = Color::hex(0x8492A8);
        cyan = Color::hex(0x0284C7); blue = Color::hex(0x2563EB); violet = Color::hex(0x7C3AED);
        pink = Color::hex(0xDB2777); amber = Color::hex(0xD97706); green = Color::hex(0x059669);
        red = Color::hex(0xDC2626); orange = Color::hex(0xEA580C); teal = Color::hex(0x0D9488);
        surface = Color::hex(0xF7F9FD); overlay = Color::hex(0xFFFFFF);
        onAccent = Color::hex(0xFFFFFF); ident = Color::hex(0x0369A1);
        unitTop = Color::hex(0xF4F7FC); unitBottom = Color::hex(0xE6EDF7); unitBorder = Color::hex(0xA9B9D3);
        wire = Color::hex(0xC3CFE2);
        shadowAlpha = 0.35f;
    }
}

inline Color phaseColor(core::Phase p) {
    switch (p) {
        case core::Phase::Fetch: return blue;
        case core::Phase::Decode: return violet;
        case core::Phase::Execute: return amber;
        case core::Phase::Memory: return pink;
        case core::Phase::WriteBack: return green;
    }
    return cyan;
}

inline Color stageColor(int s) {
    const Color c[5] = {blue, violet, amber, pink, green};
    return (s >= 0 && s < 5) ? c[s] : cyan;
}

constexpr float radius = 14.0f;
constexpr float gap = 14.0f;

} // namespace vas::ui::theme
