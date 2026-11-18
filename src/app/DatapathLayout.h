// Shared geometry of the datapath: used by the 2D datapath diagram and
// extruded into 3D by the chip view, so both always match.
#pragma once
#include <vector>

#include "core/ControlUnit.h"
#include "engine/gfx/Math.h"

namespace vas::ui::layout {

using gfx::Rect;
using gfx::Vec2;

constexpr float WORLD_W = 1100, WORLD_H = 560;

struct Block {
    int unit;
    Rect r;
};

inline const std::vector<Block>& blocks() {
    using namespace core;
    static const std::vector<Block> b = {
        {U_PC, {20, 240, 70, 56}},      {U_IMEM, {125, 196, 125, 148}}, {U_IR, {282, 238, 86, 64}},
        {U_CU, {420, 20, 222, 82}},     {U_REGS, {420, 188, 164, 192}}, {U_IMM, {420, 420, 140, 50}},
        {U_MUX, {620, 290, 32, 96}},    {U_ALU, {690, 196, 104, 208}},  {U_FLAGS, {690, 440, 112, 58}},
        {U_DMEM, {840, 196, 132, 166}}, {U_WB, {1010, 250, 32, 100}},
    };
    return b;
}

inline const Rect& blockRect(int u) {
    for (const auto& b : blocks()) if (b.unit == u) return b.r;
    return blocks()[0].r;
}

inline std::vector<Vec2> busPath(int b) {
    using namespace core;
    switch (b) {
        case B_PC_IMEM: return {{90, 268}, {125, 268}};
        case B_IMEM_IR: return {{250, 270}, {282, 270}};
        case B_PC_INC: return {{55, 240}, {55, 176}};
        case B_INC_PC: return {{31, 152}, {8, 152}, {8, 268}, {20, 268}};
        case B_IR_CU: return {{325, 238}, {325, 61}, {420, 61}};
        case B_IR_REGS: return {{368, 262}, {420, 262}};
        case B_IR_IMM: return {{325, 302}, {325, 445}, {420, 445}};
        case B_REGA_ALU: return {{584, 240}, {690, 240}};
        case B_REGB_MUX: return {{584, 318}, {620, 318}};
        case B_IMM_MUX: return {{560, 445}, {600, 445}, {600, 362}, {620, 362}};
        case B_MUX_ALU: return {{652, 338}, {690, 338}};
        case B_ALU_FLAGS: return {{745, 404}, {745, 440}};
        case B_ALU_DMEM: return {{794, 288}, {812, 288}, {812, 250}, {840, 250}};
        case B_REGB_DMEM: return {{540, 380}, {540, 410}, {826, 410}, {826, 322}, {840, 322}};
        case B_DMEM_WB: return {{972, 280}, {1010, 280}};
        case B_ALU_WB: return {{794, 318}, {802, 318}, {802, 426}, {995, 426}, {995, 330}, {1010, 330}};
        case B_WB_REGS: return {{1042, 300}, {1072, 300}, {1072, 140}, {500, 140}, {500, 188}};
        case B_FLAGS_CU: return {{745, 498}, {745, 536}, {395, 536}, {395, 84}, {420, 84}};
        case B_IR_PC: return {{300, 302}, {300, 516}, {55, 516}, {55, 296}};
        case B_DMEM_MUX: return {{905, 362}, {905, 397}, {610, 397}, {610, 378}, {620, 378}};
        default: return {};
    }
}

// ALU outline: classic V-notch shape (concave)
inline std::vector<Vec2> aluShape(const Rect& r) {
    return {{r.x, r.y}, {r.r(), r.y + r.h * 0.26f}, {r.r(), r.b() - r.h * 0.26f}, {r.x, r.b()},
            {r.x, r.y + r.h * 0.6f}, {r.x + r.w * 0.22f, r.y + r.h * 0.5f}, {r.x, r.y + r.h * 0.4f}};
}
inline std::vector<Vec2> muxShape(const Rect& r) {
    return {{r.x, r.y}, {r.r(), r.y + r.h * 0.18f}, {r.r(), r.b() - r.h * 0.18f}, {r.x, r.b()}};
}

} // namespace vas::ui::layout
