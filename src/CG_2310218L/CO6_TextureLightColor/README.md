# 2310218L.CO6 — Design the texture images, light and color using different objects.

All text is a TrueType glyph atlas texture; the 3D chip's board is a procedurally generated circuit texture with mipmaps; the chip is lit by two lights (one tinted with the current pipeline phase colour) with specular highlights, emissive glow on the active unit, and depth fog.

## Where you see it in the simulator

- Every label on screen: textured quads from the font atlas.
- 3D Chip: textured board, lighting, glow on the unit in use, fog in the distance.
- Colour coding: each phase (IF ID EX MEM WB) has its own colour in the datapath, waveform and pipeline.

## Files and functions

Paths are relative to `src/CG_2310218L/` unless they start with another top-level folder.

| File | Functions / classes | Role |
|---|---|---|
| CO6_TextureLightColor/Font.h / .cpp | `Font::load`, `draw`, `width` | glyph atlas texture (stb_truetype) |
| CO6_TextureLightColor/Texture.h / .cpp | `makePcbTexture` | procedural image + gluBuild2DMipmaps |
| CO6_TextureLightColor/Lighting.h / .cpp | `material`, `chipLights`, `depthFog` | Phong lighting and fog |

**Full explanation, worked example and viva questions:** [CO6_Explanation.pdf](CO6_Explanation.pdf)
