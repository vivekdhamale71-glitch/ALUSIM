"""In-depth explanation text for the six computer graphics course outcomes.

Each entry: (number, folder, official statement, [sections]).
A section is (heading, [blocks]). A block is one of
  ("p", text)                  a paragraph
  ("steps", [text, ...])       numbered steps
  ("code", text)               a short formula or code listing
  ("table", [header...], [[cell...], ...])
Every statement describes code that exists in src/engine/gfx or src/app.
"""

CG = [
(1, "CO1_Primitives", "Apply OpenGL graphics primitives to develop graphics applications.", [
 ("The idea", [
  ("p", "OpenGL can only draw a few basic shapes: points, lines and polygons. Everything on the screen, "
        "from a button to a wire to the 3D chip, is made of these. Our program draws all of its 2D interface "
        "with one rule: every shape is turned into a list of corner points (a polygon), and the polygon is drawn "
        "as a GL_TRIANGLE_FAN between glBegin and glEnd.")]),
 ("How a polygon reaches the screen", [
  ("steps", [
    "A drawing function such as rect(), circle() or line() works out the corner points of its shape and gives each corner a position (x, y) and a colour (r, g, b, a). This pair is a ClipVertex.",
    "The list of ClipVertex is handed to clipper::polygon(). It first checks the clipping window (CO5): a polygon fully inside is drawn as it is, one fully outside is thrown away, and one that crosses the edge is cut.",
    "emit() draws what is left: glBegin(GL_TRIANGLE_FAN), then for each corner glColor4f followed by glVertex2f, then glEnd().",
    "A triangle fan uses the first corner as the shared centre and joins every following pair of corners to it. That draws any convex polygon with n corners using n triangles.",
    "Because each corner carries its own colour, OpenGL blends the colours across the face. This is how the gradient rectangles are made."]),
  ("p", "Why a fan: a fan needs no triangulation step for a convex shape, and a quad, a circle and a rounded rectangle are all convex.")]),
 ("The primitives and how we use each", [
  ("table", ["Primitive", "OpenGL type", "Where it is used in the program"], [
    ["Point", "GL_POINTS (with glPointSize and GL_POINT_SMOOTH)", "Draw.cpp points(): the dots that flow along the buses and the LED rings. Each point is tested against the clipping window before it is sent."],
    ["Quad", "GL_TRIANGLE_FAN with 4 corners", "Draw.cpp rect(), rectGrad(), rectGradH(): every panel, bar and background. A gradient only changes the colour of two corners."],
    ["Thick line", "a quad built along the line", "Draw.cpp line(): the line is turned into a rectangle w pixels wide. The perpendicular of the direction, scaled to half the width, is added to and taken from both ends to give the four corners."],
    ["Triangle", "GL_TRIANGLE_FAN with 3 corners", "Draw.cpp triangle() and arrowHead(): the arrow tips on the wires."],
    ["Circle", "fan of seg corners on a circle", "Draw.cpp circle(): corner i is (cx + r cos a, cy + r sin a) where a = 2 pi i / seg."],
    ["Ring", "seg quads between two radii", "Draw.cpp ring(): one quad per segment between radius r and r - w."],
    ["Rounded rectangle", "fan over a rounded outline", "Draw.cpp roundPath(), roundRect(), roundRectOutline(): four quarter-circle arcs joined by the straight sides."],
    ["Convex polygon", "fan of n corners", "Draw.cpp convexPolygon(): the ALU shape and the multiplexer shape."],
    ["3D box", "GL_QUADS, 6 faces, a normal for each", "Extrude.cpp box() (the units and board of the 3D chip) and Scene3D.cpp box() (the pipeline, data and arithmetic views). Each face has its own glNormal3f so lighting (CO6) works."]]),
  ("p", "A rounded rectangle uses fewer corners on small radii (2 segments under 4 pixels, 5 under 10, otherwise 8) so tiny buttons are not made of hundreds of triangles.")]),
 ("A thick line, worked out", [
  ("p", "Draw the line from a = (10, 10) to b = (110, 10) with width 4."),
  ("steps", [
    "The direction d = b - a = (100, 0) and its length is 100.",
    "The perpendicular of d is (0, 100); divided by the length it is (0, 1); scaled by half the width (2) it is n = (0, 2).",
    "The four corners are a + n = (10, 12), b + n = (110, 12), b - n = (110, 8) and a - n = (10, 8).",
    "These four corners are drawn as one polygon. The result is a rectangle 100 long and 4 thick, and it works for any angle."])]),
 ("Where it is implemented", [
  ("table", ["File", "What is in it"], [
    ["engine/gfx/Draw.cpp", "Every 2D primitive listed above: rect, line, circle, ring, rounded shapes, dashed lines, bezier curves, points, text."],
    ["engine/gfx/Scene3D.cpp", "The 3D box, a rotated box and the floor quad."],
    ["engine/gfx/Extrude.cpp", "box() and prism() for the 3D chip."],
    ["CG_2310218L/CO1_Primitives/gl_primitives.cpp", "One function for each OpenGL primitive type: points, lines, strips, loops, triangles, fans, quads and polygons."],
    ["CG_2310218L/CO1_Primitives/ui_shapes.cpp", "The interface shapes built from them: gradients, rounded rectangles, circles, rings and dashed lines."],
    ["CG_2310218L/CO1_Primitives/chip_board.cpp", "A board with registers, an ALU shape, buses and flag lamps drawn only from primitives."]])]),
]),

(2, "CO2_Transformations", "Solve the real time problems using geometric transformations.", [
 ("The idea", [
  ("p", "A transformation moves every point of a picture by the same rule. In 2D each rule is a 3x3 matrix, and a point is written as (x, y, 1) so that a move (translation) can also be a matrix. "
        "Several transformations are combined by multiplying their matrices once, then the single result is applied to every point.")]),
 ("The matrices", [
  ("code", "translate(tx, ty)   | 1 0 tx |      scale(sx, sy)   | sx 0  0 |      rotate(a)   | cos a  -sin a  0 |\n"
           "                    | 0 1 ty |                      | 0  sy 0 |                  | sin a   cos a  0 |\n"
           "                    | 0 0 1  |                      | 0  0  1 |                  |   0       0    1 |"),
  ("p", "Mat3 in engine/gfx/Math.h stores the nine numbers row by row. translate() puts tx and ty in the last column, scale() puts sx and sy on the diagonal, and rotate() fills the top-left 2x2 block with cos and sin. "
        "apply() computes x' = m0 x + m1 y + m2 and y' = m3 x + m4 y + m5."),
  ("p", "Reflection is a scale by -1 on one axis, and shear puts a number off the diagonal. Those two are written in the CO2 folder, transforms_2d.cpp, with the other three.")]),
 ("Combining by multiplication", [
  ("p", "Mat3::operator* multiplies row i of the left matrix by column j of the right one. The order matters: (A * B) applies B first, then A. "
        "That is why the program writes a view as a product of three matrices, read from right to left."),
  ("code", "M = translate(centre + pan)  *  scale(s, s)  *  translate(-world centre)\n"
           "      3: move to the screen       2: zoom        1: put the world centre at the origin first"),
  ("p", "Zooming about a point needs the origin to be that point, so the picture is moved there first (step 1), scaled (step 2) and moved back (step 3). This is the same pattern as rotating about any pivot.")]),
 ("Pan and zoom of the datapath, step by step", [
  ("steps", [
    "fit = the largest scale at which the whole datapath (WORLD_W by WORLD_H) fits in the window: min(view.w / WORLD_W, view.h / WORLD_H). The user zoom is multiplied on top of it.",
    "Dragging with the mouse changes pan_ by the number of pixels the mouse moved since the button went down.",
    "Scrolling the wheel changes the zoom by a factor of 1.12 (or 1/1.12), limited to 0.6 to 3.5.",
    "To keep the point under the mouse still, the pan is corrected: pan' = pan + (mouse - centre - pan) * (1 - newZoom / oldZoom). Without this the picture would zoom about the middle of the window and jump away from the cursor.",
    "Every wire corner is sent through M with apply() to get its pixel position before it is clipped (CO5) and drawn.",
    "inverseAffine() builds the opposite transformation. For a matrix [A t; 0 1] the inverse is [A^-1  -A^-1 t; 0 1], with A^-1 found from the 2x2 determinant. It turns a mouse position back into world coordinates."]),
  ("p", "Check of the zoom correction: let the centre be 0, pan 0, zoom 1, and the mouse at x = 100. A world point at 100 is under the mouse. Zooming to 2 gives pan' = 0 + (100 - 0 - 0) * (1 - 2) = -100. The point is now drawn at 2 * 100 - 100 = 100, the same place as before.")]),
 ("Hierarchical movement with the matrix stack (3D)", [
  ("p", "In 3D, OpenGL keeps a current matrix. glTranslatef, glRotatef and glScalef multiply it, and glPushMatrix and glPopMatrix save it and restore it. "
        "A child object is drawn after its parent's transformations, so it moves with its parent."),
  ("steps", [
    "glPushMatrix() saves the matrix of the parent.",
    "glTranslatef / glRotatef position the child relative to the parent.",
    "The child is drawn.",
    "glPopMatrix() restores the parent's matrix, so the next child starts from the same place."]),
  ("p", "Scene3D.cpp boxRotX() uses this: it pushes, translates to the centre of the box, rotates about the x axis, draws the box and pops. The arithmetic view uses it to flip the bit cubes. "
        "hierarchy_transforms.cpp in the CO folder shows the full chain with an arm whose forearm hangs from the upper arm.")]),
 ("Where it is implemented", [
  ("table", ["File", "What is in it"], [
    ["engine/gfx/Math.h, Math.cpp", "Mat3: translate, scale, rotate, multiply, apply, scaleX and inverseAffine."],
    ["app/DatapathView.cpp", "The view matrix M, drag to pan, wheel zoom about the cursor, and the world to screen mapping of every wire."],
    ["engine/gfx/Scene3D.cpp", "boxRotX(): push, translate, rotate, draw, pop."],
    ["CG_2310218L/CO2_Transformations/transforms_2d.cpp", "All five 2D transformations as matrices, multiplication, and rotation or scaling about a pivot."],
    ["CG_2310218L/CO2_Transformations/pan_zoom_view.cpp", "The view with pan, zoom about the mouse, its inverse and fit to window."],
    ["CG_2310218L/CO2_Transformations/hierarchy_transforms.cpp", "Objects placed relative to a parent with the matrix stack."]])]),
]),

(3, "CO3_ObjectAlgorithms", "Develop 2D and 3D objects using various algorithms.", [
 ("The idea", [
  ("p", "A screen is a grid of pixels, so a line or a circle has to be turned into the list of pixels that lie closest to it. "
        "The algorithms below do this with only additions and comparisons. A 3D solid is made from a 2D outline by extruding it (sweeping it along a straight path).")]),
 ("Bresenham line", [
  ("p", "Draws a line between two pixels using integers only. The error term tells how far the true line has drifted from the pixel row."),
  ("steps", [
    "dx = |x1 - x0|, dy = -|y1 - y0|, and each step direction sx, sy is +1 or -1 towards the end point. error = dx + dy.",
    "Plot (x0, y0). If it is the end point, stop.",
    "e2 = 2 * error. If e2 >= dy the line has moved enough in x: error += dy and x0 += sx.",
    "If e2 <= dx the line has moved enough in y: error += dx and y0 += sy. Go back to the plot step."]),
  ("p", "Worked example, (0, 0) to (5, 2): dx = 5, dy = -2, error = 3. Plot (0,0): e2 = 6, so x moves and error = 1. Plot (1,0): e2 = 2, so x moves (error = -1) and, because e2 <= dx, y moves too (error = 4). "
        "Plot (2,1), then (3,1), (4,2) and (5,2). The pixels are (0,0) (1,0) (2,1) (3,1) (4,2) (5,2), and no floating point number was used. "
        "This function is used in DatapathView to put the flowing dots on the exact pixels of a bus."),
  ("p", "The DDA line in the CO folder does the same with floating point: it takes steps = max(|dx|, |dy|) steps and adds dx / steps and dy / steps each time. It is simpler but slower because of the rounding.")]),
 ("Midpoint circle", [
  ("p", "Draws one eighth of a circle, from the top (0, r) to the 45 degree point, and copies each pixel into the other seven by symmetry."),
  ("steps", [
    "Start at x = 0, y = r with decision d = 1 - r.",
    "Plot the eight symmetric pixels (+-x, +-y) and (+-y, +-x) around the centre.",
    "If d < 0 the midpoint between the two candidates is inside the circle, so the next pixel is east: d += 2x + 3.",
    "Otherwise it is outside, so the next pixel is south-east: d += 2(x - y) + 5 and y decreases by one.",
    "x increases by one. Repeat while x <= y."]),
  ("p", "Worked example with r = 3: start (0,3) with d = -2. Plot (0,3); d < 0 so d = 1 and x = 1. Plot (1,3); d >= 0 so d = 1 + 2(1 - 3) + 5 = 2, y = 2 and x = 2. Plot (2,2); the next x is 3, which is more than y = 2, so the loop ends. "
        "One eighth is (0,3), (1,3), (2,2), and the symmetry gives the other seven eighths. "
        "In the program this draws the ring around each flag lamp (CF, ZF, SF, OF) in Common.cpp and in DatapathView.cpp, as GL_POINTS."),
  ("p", "The midpoint ellipse in the CO folder has two regions: where the curve is flatter than 45 degrees it steps in x, then where it is steeper it steps in y, and each pixel gives four copies.")]),
 ("Scan-line polygon fill", [
  ("steps", [
    "Find the lowest and highest y of the polygon.",
    "For every pixel row, take the height at the middle of the row (row + 0.5) so that a vertex never lies exactly on a row.",
    "Find every edge that crosses this height (one end below, one end above). Its x is a.x + t (b.x - a.x) with t = (y - a.y) / (b.y - a.y).",
    "Sort the crossings from left to right. The first two are the start and end of an inside span, the next two another, and so on (inside, outside, inside ...).",
    "Each span is returned as (row, x0, x1) and drawn as a strip one pixel high."]),
  ("p", "In the program, fillScanline() in DatapathView.cpp uses these spans to paint the ALU shape with a vertical gradient: each strip gets a colour between the top and the bottom colour by its row.")]),
 ("Boundary fill and flood fill", [
  ("p", "Both start from a seed pixel and spread to the four neighbours with a stack. Boundary fill stops at pixels of the boundary colour (so it fills the inside of an outline). "
        "Flood fill replaces one old colour with a new one and stops at any other colour (so it fills a region of equal colour). They are written in polygon_fill.cpp.")]),
 ("3D solids by extrusion", [
  ("steps", [
    "The 2D outline is a list of corners on the floor, listed counter-clockwise.",
    "Each edge a to b becomes a wall: a quad with corners a at the bottom, b at the bottom, b at the top, a at the top.",
    "The normal of the wall is the edge direction turned a quarter turn. It is needed for lighting.",
    "The top is closed by a triangle fan from a chosen centre point to every corner."]),
  ("p", "In the program prism() in Extrude.cpp extrudes the ALU and multiplexer outlines on the 3D chip, and box() extrudes a rectangle for the other units. "
        "The CO folder also revolves a profile around the y axis (a vase): each pair of profile points and angles makes one quad.")]),
 ("Where it is implemented", [
  ("table", ["File", "What is in it"], [
    ["engine/gfx/Raster.cpp", "bresenhamLine, midpointCircle and scanlineFill."],
    ["app/DatapathView.cpp, app/Common.cpp", "Where those three are called: bus dots, flag rings and the gradient ALU."],
    ["engine/gfx/Extrude.cpp", "prism() and box()."],
    ["CG_2310218L/CO3_ObjectAlgorithms/line_circle_algorithms.cpp", "DDA and Bresenham lines, midpoint circle and midpoint ellipse."],
    ["CG_2310218L/CO3_ObjectAlgorithms/polygon_fill.cpp", "Scan-line fill, boundary fill and flood fill."],
    ["CG_2310218L/CO3_ObjectAlgorithms/extrusion_lathe.cpp", "Extrusion of an outline and revolution of a profile."]])]),
]),

(4, "CO4_ProjectionViewing", "Apply various methods for projection and viewing it into real world.", [
 ("The idea", [
  ("p", "Projection turns 3D points into 2D screen points. Perspective makes far things smaller, as the eye sees. Parallel (orthographic) projection keeps sizes the same at every distance. "
        "Viewing places the camera: where it stands, where it looks and which way is up.")]),
 ("Viewing: the orbit camera", [
  ("p", "The camera stands on a sphere around a target point and always looks at it. The position is worked out from two angles and a distance."),
  ("code", "eye.x = target.x + d * cos(pitch) * sin(yaw)\neye.y = target.y + d * sin(pitch)\neye.z = target.z + d * cos(pitch) * cos(yaw)\n"
           "gluLookAt(eye, target, up = (0, 1, 0))"),
  ("p", "Dragging the mouse changes yaw (left and right) and pitch (up and down), and the wheel changes d. gluLookAt builds the view matrix in GL_MODELVIEW from the eye, the target and the up direction. "
        "In 2D mode the pitch is set to 89.5 degrees so the camera looks straight down on the chip.")]),
 ("Projection: perspective and parallel", [
  ("steps", [
    "glMatrixMode(GL_PROJECTION) and glLoadIdentity() select and clear the projection matrix.",
    "Perspective: gluPerspective(fov, aspect, near, far) builds a view volume shaped like a pyramid with its tip at the eye. A point is divided by its depth, so the same object is smaller when it is further away.",
    "Parallel: glOrtho(-s*aspect, s*aspect, -s, s, -1000, 1000) builds a box-shaped volume. The depth is not used for the size.",
    "Objects outside the volume are clipped by OpenGL, and the rest are mapped to the viewport."])]),
 ("The dolly zoom between the two", [
  ("p", "The 3D / 2D button animates from perspective to parallel. orbitCamera() takes a value f from 0 (perspective) to 1 (parallel) and does a dolly zoom:"),
  ("steps", [
    "The field of view shrinks from 38 degrees to 2.5 degrees as f goes from 0 to 1.",
    "The camera distance is made longer so that the height of the view at the target, d * tan(fov / 2), stays the same. The chip keeps its size on screen.",
    "As the field of view gets narrow and the eye gets far away, the projection lines become nearly parallel.",
    "At f > 0.985 the camera switches to glOrtho with the same view height, so the change cannot be seen."]),
  ("p", "Numbers: with fov 38 and distance 20 the view height is 20 * tan(19 degrees) = 6.9. With fov 2.5 the distance becomes 6.9 / tan(1.25 degrees) = 316, and the height is again 6.9.")]),
 ("Viewport and scissor", [
  ("p", "A view is drawn only inside its own rectangle of the window. glViewport maps the projected scene to that rectangle, and glScissor with GL_SCISSOR_TEST stops anything from being drawn outside it. "
        "Scene3D::begin() and the chip view both set them (with the y axis flipped, because the window's y grows downwards and OpenGL's grows upwards), then clear the depth buffer and enable GL_DEPTH_TEST with GL_LEQUAL so near surfaces hide far ones."),
  ("p", "project() in Scene3D.cpp does the opposite with gluProject: a 3D point goes through the modelview and projection matrices and the viewport to give a 2D pixel, used to place text labels over 3D objects.")]),
 ("Where it is implemented", [
  ("table", ["File", "What is in it"], [
    ["engine/gfx/Camera.cpp", "orbitCamera(): gluLookAt, gluPerspective, glOrtho and the dolly zoom."],
    ["engine/gfx/Scene3D.cpp", "begin(): viewport, scissor, depth test, perspective and look-at for the pipeline, arithmetic and data views; project()."],
    ["app/ChipView.cpp", "Uses orbitCamera() for the 3D chip and the 3D / 2D switch."],
    ["CG_2310218L/CO4_ProjectionViewing/", "This PDF; the projection code itself is in the engine files above."]])]),
]),

(5, "CO5_Clipping", "Implement the suitable clipping algorithms.", [
 ("The idea", [
  ("p", "Clipping removes the part of a picture that lies outside a window. The datapath is drawn inside a panel; when it is zoomed in, most wires are outside the panel and must not be drawn or must be cut at its edge. "
        "The program does this in software, before OpenGL draws anything (the 2D scissor test is disabled on purpose).")]),
 ("Cohen-Sutherland line clipping", [
  ("steps", [
    "Give every end point a 4-bit outcode: one bit for left, right, top and bottom of the window. Inside has code 0000. (The window's y grows downwards, so top is y < window.y.)",
    "If both codes are 0000, the line is inside: accept it.",
    "If the two codes share a bit (code A AND code B is not 0), both ends are beyond the same edge: reject the line.",
    "Otherwise pick an end point that is outside, and move it to where the line crosses the first edge it is beyond, for example for the top edge x = a.x + (b.x - a.x)(top - a.y)/(b.y - a.y).",
    "Recalculate the code of the moved point and go back to step 2."]),
  ("p", "Worked example: window x from 0 to 10, y from 0 to 10; line (-5, 5) to (5, 5). The code of (-5, 5) is LEFT, the code of (5, 5) is 0. Not both 0, no shared bit. The left end is moved to x = 0 on the same line, giving (0, 5). Now both codes are 0: accept (0, 5) to (5, 5)."),
  ("p", "It is used in DatapathView.cpp to clip every bus segment before drawing it, and in Draw::line() through lineClass() to reject lines that are completely outside early. For a thick line the window is grown or shrunk by half the width so the line's edges are not cut off.")]),
 ("Liang-Barsky line clipping", [
  ("p", "Writes the line as a point a + t (b - a) for t from 0 to 1 and finds the part of t that is inside all four edges. For each edge k there is a value p_k and q_k with the rule p_k * t <= q_k."),
  ("code", "p = (-dx, dx, -dy, dy)      q = (a.x - left, right - a.x, a.y - top, bottom - a.y)"),
  ("steps", [
    "Start with t0 = 0 and t1 = 1.",
    "If p = 0 the line is parallel to that edge. If q < 0 it is outside: reject.",
    "If p < 0 the line enters through this edge: t0 = max(t0, q / p). If p > 0 it leaves: t1 = min(t1, q / p).",
    "If at any time t0 > t1 the line misses the window: reject.",
    "The clipped line runs from a + t0 (b - a) to a + t1 (b - a)."]),
  ("p", "Liang-Barsky needs only one division per edge and no repeated outcode tests. The program uses it for the plot lines in DatapathView.cpp, and dashedLine() uses the value t0 and t1 to make dashes only on the visible part of a line, so a zoomed-in dashed wire does not waste time on dashes far off screen.")]),
 ("Sutherland-Hodgman polygon clipping", [
  ("p", "Clips a polygon against the window one edge at a time. The output of one edge is the input of the next."),
  ("steps", [
    "For each window edge (left, right, top, bottom), go round the polygon one edge at a time, from the previous corner to the current corner.",
    "In to in: keep the current corner.",
    "In to out: keep only the point where the edge crosses the window edge.",
    "Out to in: keep the crossing point, then the current corner.",
    "Out to out: keep nothing.",
    "After the four window edges the list is the clipped polygon (empty if nothing was inside)."]),
  ("p", "The crossing point is found from the signed distances dp and dq of the two corners to the window edge: t = dp / (dp - dq). Every attribute (x, y, red, green, blue, alpha, and the texture coordinates u and v) is interpolated with the same t, so a clipped gradient or a clipped text glyph keeps its right colour and texture.")]),
 ("The clipping stage every shape passes through", [
  ("steps", [
    "Every filled shape is sent to clipper::polygon().",
    "The bounding box of the polygon is found.",
    "If the box is inside the window: count it as accepted and draw it as it is.",
    "If the box is completely outside: count it as rejected and draw nothing.",
    "Otherwise run Sutherland-Hodgman, count it as clipped and draw the result."]),
  ("p", "The counts of accepted, rejected and clipped shapes are kept in Stats, so the program can show how much work the clipping saved. pushClip() and popClip() in Draw.cpp set the window, and nested windows are handled by a stack.")]),
 ("Where it is implemented", [
  ("table", ["File", "What is in it"], [
    ["engine/gfx/Clip.cpp", "outCode, cohenSutherland, liangBarskyT and liangBarsky."],
    ["engine/gfx/PolygonClip.cpp", "sutherlandHodgman with the interpolation of all attributes."],
    ["engine/gfx/Clipper.cpp", "The stage: accept, reject or clip every polygon, the line classifier and the counters."],
    ["engine/gfx/Draw.cpp", "Calls the stage from every shape, and uses Liang-Barsky for dashed lines."],
    ["app/DatapathView.cpp", "Clips the bus segments with Cohen-Sutherland and the plot lines with Liang-Barsky."]])]),
]),

(6, "CO6_TextureLightColor", "Design the texture images, light and color using different objects.", [
 ("The idea", [
  ("p", "Colour, light and texture decide how a surface looks. A texture is an image wrapped on a surface; a light and a material decide how bright each point is; fog and blending change colours with distance and transparency.")]),
 ("Textures", [
  ("steps", [
    "makePcbTexture() builds a 512 by 512 picture in memory, pixel by pixel, with no image file: a dark blue base with a little random noise, a bright line every 32 pixels for the grid, 70 copper traces (horizontal or vertical runs 3 pixels wide) and a small round via at the end of each trace.",
    "glGenTextures and glBindTexture create and select a texture object.",
    "glTexParameteri sets how it is sampled: GL_LINEAR_MIPMAP_LINEAR when it is made smaller, GL_LINEAR when it is made bigger, and GL_REPEAT so the image tiles.",
    "gluBuild2DMipmaps uploads it together with all the smaller copies (256, 128, 64 ... 1 pixels). A mipmap is a pre-shrunk copy, so a texture far away is read from a small copy and does not shimmer.",
    "Each corner of a surface gets a texture coordinate (u, v) from 0 to 1 with glTexCoord2f, and OpenGL finds the colour in between."]),
  ("p", "The text of the whole interface is a texture too. Font.cpp draws every letter once into one big atlas (a single-channel GL_ALPHA texture), with GL_CLAMP_TO_EDGE so letters do not bleed into each other. "
        "Each letter is then drawn as a textured quad that picks its own part of the atlas. The alpha of the texture is the shape of the letter, and the colour is set by the program, so one atlas serves every colour.")]),
 ("Light and material", [
  ("p", "With GL_LIGHTING on, the colour of a point is worked out from its normal, the lights and the material, not from glColor."),
  ("table", ["Part", "What it does", "In the program"], [
    ["Ambient", "Light that comes from everywhere; the shadowed side is never black.", "Material ambient is 45 percent of the colour; light 0 ambient is (0.25, 0.28, 0.35)."],
    ["Diffuse", "Brightness depends on the angle between the normal and the light: n . l.", "Material diffuse is the colour itself."],
    ["Specular", "A bright highlight where the surface mirrors the light to the eye, sharper with a higher shininess.", "A pale blue-grey specular and a shininess of 60 for the chip units."],
    ["Emission", "The surface gives off its own light (it does not light others).", "A unit glows with the colour of the current phase while it is active."],
    ["Directional light", "Position with w = 0: parallel rays, like the sun.", "Light 0 at (-0.4, 1, 0.6, 0)."],
    ["Point light", "Position with w = 1: rays spread from a point, getting weaker with distance (quadratic attenuation 0.004).", "Light 1 at (6, 5, -4, 1), coloured by the current phase."]]),
  ("p", "GL_NORMALIZE keeps the normals one unit long after scaling, and GL_SMOOTH shading blends the lit colour between corners. The chip's lights use the pipeline-phase colour, so the whole die is tinted by what the processor is doing (fetch, decode, execute ...)."),
  ("p", "The lit value of one point, in short: colour = emission + ambient light x ambient material + diffuse light x diffuse material x max(n . l, 0) + specular light x specular material x max(r . v, 0)^shininess.")]),
 ("Fog", [
  ("p", "depthFog() uses linear fog: the final colour is mixed between the object's colour and the fog colour by f = (end - z) / (end - start), limited to 0 to 1, where z is the distance from the eye."),
  ("p", "The 3D chip uses start = 0.9 x the camera distance and end = 2.6 x the camera distance, with the fog colour equal to the background, so far parts fade away. Fog is turned off in the flat 2D view because there every part has the same depth.")]),
 ("Colour and blending", [
  ("p", "Colours are four numbers (red, green, blue, alpha) from 0 to 1. Color::lerp() mixes two colours by t, and the gradients and animations use it. "
        "With glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA) a new pixel is mixed with the pixel already there: result = new x alpha + old x (1 - alpha). This gives transparent panels and soft shadows."),
  ("p", "For glows the program switches to glBlendFunc(GL_SRC_ALPHA, GL_ONE), which adds the new colour to the old (additive blending), so overlapping glows get brighter. Depth writing is switched off (glDepthMask(GL_FALSE)) while drawing them so they do not hide what is behind."),
  ("p", "Worked example of the first blend: a red panel (1, 0, 0) with alpha 0.25 on a blue background (0, 0, 1) gives (0.25, 0, 0.75).")]),
 ("Where it is implemented", [
  ("table", ["File", "What is in it"], [
    ["engine/gfx/Texture.cpp", "makePcbTexture(): the generated image, mipmaps, filtering and wrapping."],
    ["engine/gfx/Font.cpp", "The glyph atlas texture and the textured quads for text."],
    ["engine/gfx/Lighting.cpp", "material(), chipLights() (directional and point light) and depthFog()."],
    ["engine/gfx/Scene3D.cpp", "The lights and materials of the pipeline, arithmetic and data views."],
    ["engine/gfx/Draw.cpp, app/ChipView.cpp", "Alpha blending and additive glow."],
    ["app/ChipView.cpp", "Applies the texture to the board, the lights and the fog to the 3D chip."]])]),
]),
]
