"""Extra text for the PDFs: how each CO was implemented."""

HOW = {
    ("PL", 1): "We implemented arrays three ways: a fixed-size register file with checked access, the basic array "
               "operations with their cost counted in element moves, and a two-dimensional array for tables.",
    ("PL", 2): "We implemented the three kinds of linked list, each shaped for one job: a singly linked list for a "
               "token stream, a doubly linked list for an undo history and a circular list for the repeating "
               "instruction phases.",
    ("PL", 3): "We implemented the stack and the queue by hand on arrays and applied them in four scenarios: matching "
               "brackets, a calculator that converts infix to postfix, undo and redo, and a circular queue with "
               "Round Robin scheduling.",
    ("PL", 4): "We implemented six sorting and five searching algorithms that count their own work, and two "
               "applications that combine them: an opcode table (sort once, binary search many times) and memory "
               "shown in address order.",
    ("PL", 5): "We implemented hashing with four collision strategies, a symbol table that grows by rehashing, an "
               "LRU cache and three problems that a hash table solves in one pass.",
    ("COA", 1): "We implemented the processor's register views and flags, every addressing mode with the stack, and "
                "an ALU routine that runs each operation on the real processor and returns the flags.",
    ("COA", 2): "We implemented assembly programs in four groups: arrays, numbers, strings, and comparisons with "
                "division.",
    ("COA", 3): "We implemented Booth's and add-and-shift multiplication, restoring and non-restoring division, and "
                "the number conversions, all on n-bit registers.",
    ("COA", 4): "We implemented a hardwired decoder, a micro-programmed control store with a dispatch table, and the "
                "timing rules of a five-stage pipeline.",
    ("CG", 1): "We implemented every drawing from OpenGL primitives: one function per primitive type, the shapes of "
               "an interface built from them, and a CPU board.",
    ("CG", 2): "We implemented the geometric transformations as matrices, a pan and zoom view with its inverse, and "
               "hierarchical movement with the matrix stack.",
    ("CG", 3): "We implemented lines, circles and ellipses pixel by pixel, three ways of filling an area, and 3D "
               "solids made by extrusion and revolution.",
    ("CG", 4): "We implemented the projections, an orbit camera with a dolly zoom, and four viewports for one scene.",
    ("CG", 5): "We implemented line clipping, polygon clipping and a clipping stage that every shape passes through.",
    ("CG", 6): "We implemented lights and materials, generated textures with mipmaps, fog, the HSV colour model and "
               "alpha blending.",
    ("PSOOP", 1): "We implemented real-world objects as classes with private data: a bank (inheritance and "
                  "composition), a library (objects that hold objects) and a translator from English sentences to "
                  "program statements.",
    ("PSOOP", 2): "We implemented an instruction hierarchy that runs through one base class, two control units "
                  "behind one interface, an exception hierarchy caught in one place, and operator overloading for "
                  "registers and vectors.",
    ("PSOOP", 3): "We implemented a small compiler in three steps (lexer, shunting-yard parser, code generator), a "
                  "maze solver with breadth-first search and N-Queens with backtracking.",
    ("PSOOP", 4): "We implemented the observer, command with undo and redo, singleton, factory, friend, static member, "
                  "strategy and state patterns.",
}
