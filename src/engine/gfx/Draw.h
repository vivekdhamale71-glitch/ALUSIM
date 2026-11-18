// =====================================================================
//  2D drawing on top of OpenGL primitives                 [CG CO1]
//  GL_TRIANGLE_FAN / GL_TRIANGLE_STRIP / GL_QUADS / GL_POINTS build
//  every widget: rounded cards, gradients, soft shadows, glows, thick
//  anti-aliased lines, Bezier curves and the toolbar icons.
// =====================================================================
#pragma once
#include <string>
#include <vector>

#include "engine/gfx/Font.h"
#include "engine/gfx/Math.h"

namespace vas::gfx {

enum class Align { Left, Center, Right };

namespace draw {

void beginFrame(int w, int h);
void setLightMode(bool light);   // glows use additive blending only on dark backgrounds
int screenW();
int screenH();

void rect(const Rect& r, const Color& c);
void rectGrad(const Rect& r, const Color& top, const Color& bottom);
void rectGradH(const Rect& r, const Color& left, const Color& right);
void roundRect(const Rect& r, float radius, const Color& c);
void roundRectGrad(const Rect& r, float radius, const Color& top, const Color& bottom);
void roundRectOutline(const Rect& r, float radius, const Color& c, float width = 1.0f);
void shadow(const Rect& r, float radius, float spread, const Color& c);
void glow(const Rect& r, float radius, float spread, const Color& c);

void line(Vec2 a, Vec2 b, const Color& c, float width = 1.5f);
void polyline(const std::vector<Vec2>& pts, const Color& c, float width = 1.5f);
void dashedLine(Vec2 a, Vec2 b, const Color& c, float width, float dash, float gap, float offset = 0);
void bezier(Vec2 p0, Vec2 p1, Vec2 p2, Vec2 p3, const Color& c, float width = 2.0f, int segments = 32);
Vec2 bezierPoint(Vec2 p0, Vec2 p1, Vec2 p2, Vec2 p3, float t);
void circle(Vec2 c, float r, const Color& col, int segments = 32);
void ring(Vec2 c, float r, float width, const Color& col, int segments = 40);
void triangle(Vec2 a, Vec2 b, Vec2 c, const Color& col);
void convexPolygon(const std::vector<Vec2>& pts, const Color& col);
void arrowHead(Vec2 tip, Vec2 dir, float size, const Color& c);
void points(const std::vector<Vec2>& pts, float size, const Color& c);

// text
float text(Face f, int px, float x, float y, const std::string& s, const Color& c, Align a = Align::Left);
float textWidth(Face f, int px, const std::string& s);
// draws text vertically centred in r
float textIn(Face f, int px, const Rect& r, const std::string& s, const Color& c, Align a = Align::Left, float pad = 0);
// word-wrapped paragraph; returns the height used
float paragraph(Face f, int px, const Rect& r, const std::string& s, const Color& c, float lineSpacing = 1.35f);
std::string ellipsize(Face f, int px, const std::string& s, float maxW);

// clipping with the scissor test (nests)
void pushClip(const Rect& r);
void popClip();

// icons (drawn from primitives)
enum class Icon { Play, Pause, Step, Back, Reset, Finish, Chip, Bolt, Layers, Grid, Cube, Database, Code, Check, Cross,
                  Plus, Minus, ChevronL, ChevronR, Save, Folder, Export, Sun, Moon, Book };
void icon(Icon i, const Rect& r, const Color& c);

} // namespace draw
} // namespace vas::gfx
