// =====================================================================
//  Virtual ALU Simulator
//  Type a statement -> watch it compile to x86-64 -> run it on an
//  animated datapath, pipeline and 3D chip.
//
//  Second Year B.Tech (Software Engineering) mini project integrating
//    PL    (2310214L)  data structures: arrays, linked lists, stack,
//                      queue, searching, sorting, hashing
//    COAL  (2310221L)  x86 registers & instruction set, assembly,
//                      computer arithmetic, control unit, pipelining
//    CGL   (2310218L)  OpenGL primitives, transformations, 2D/3D
//                      objects, projection, clipping, texture & light
//    PSOOP (2310261L)  classes, inheritance, polymorphism, templates,
//                      operator overloading, exceptions, patterns
// =====================================================================
#include "app/App.h"

int main(int argc, char** argv) {
    vas::ui::App& app = vas::ui::App::get();
    app.init(argc, argv);
    app.run();
    return 0;
}
