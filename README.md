# Virtual ALU Simulator

**Write plain English or C++, watch it turn into x86-64 assembly, and see the CPU run it.** For example, `add 12 and 3`, `if marks is at least 40 then set pass to 1 otherwise set pass to 0`, `find the factorial of 5`, or `for (int i = 0; i < n; i++) sum += i;`. The simulator animates every step: the translation, the parse tree, the datapath, the control signals, the pipeline, and a 3D chip.

Second Year B.Tech (Software Engineering) mini project. It integrates **PL (2310214L)**, **COAL (2310221L)**, **CGL (2310218L)** and **PSOOP (2310261L)**.

Team: Vivek Dhamale · Pranjal Bhandari · Sanika Patil · Mrudula Bongulwar

---

## Run it

```bash
sudo apt install g++ nasm freeglut3-dev libglu1-mesa-dev   # once (freeglut3-dev also brings libx11-dev, used for paste)
./build.sh run                                             # build + start the simulator
./build.sh test                                            # 131 unit tests for the core
python3 docs/make_co_pdfs.py                               # rebuild the CO explanation PDFs (needs Chrome)
```

There is also a `Makefile` (`make run`, `make test`). On Windows (MSYS2 MinGW64), install `mingw-w64-x86_64-freeglut` and `mingw-w64-x86_64-nasm` and run `make OS=Windows_NT`.

## Controls

| Key | Action |
|---|---|
| Ctrl+Enter | Compile & load the editor contents |
| Space / F5 | Run / pause |
| → / F10 | Step one instruction (animated micro-steps) |
| ← / F9 | Step back (undo through the history list) |
| F1 … F6 | Switch screens |
| ☀ / ☾ button (top bar) | Dark / light theme |
| F11 | Full screen |
| Ctrl+V / Shift+Insert | Paste the clipboard into the code editor (click the editor first) |
| Ctrl+S / Ctrl+O | Save / load `my_program.txt` |
| Ctrl+E | Export the execution trace to `trace_log.txt` |
| Mouse wheel / drag | Zoom & pan the datapath, orbit the 3D chip |

## The six screens

Every screen works in a **dark** and a **light** theme. 3D is used where depth adds information: the 3D chip, the **3D pipeline conveyor** (instruction blocks moving across five stage platforms), the **3D register cubes** in Arithmetic (bits flip and slide), and the **3D sorting bars** in the Data Lab. All of them share `CG_2310218L/CO4_ProjectionViewing/Scene3D.cpp`.



1. **Compiler**: three input modes, **English | C++ | Assembly**.
   - **English**: a rule-based translator (`PSOOP_2310261L/CO1_RealWorldOOP/English.cpp`) turns sentences into statements and shows each word's role (verb, number, variable, operator, destination) and the rule it matched. Supported sentences include: add / subtract / multiply / divide, set … to, increase / decrease / double / swap, if … then … otherwise, repeat N times, while, find the factorial / largest / sum from A to B, raise to the power, and multiply using repeated addition.
   - **C++**: `int` declarations, `+= -= *= …`, `++`/`--`, `if/else`, `while`, `for`, and `int main() { … }`. `#include`, `using`, `return` and `cout` are accepted and skipped.
   - Then the statement → assembly animation runs. The lexer turns text into tokens, keywords are found by binary search, the shunting-yard algorithm converts the expression (operator stack and output queue), code generation uses a value stack and register allocation, and variables are hashed into the symbol table. In Assembly mode you can write x86-64 directly.
2. **Datapath**: the full CPU datapath with data packets moving along the wires, the control-signal vector for every cycle, the hardwired decoder table or the micro-code control store, and a logic-analyzer waveform.
3. **Pipeline**: a 5-stage pipeline built from the real execution trace. It shows stalls, flushes and forwarding paths, with cycles, CPI and speed-up compared against a non-pipelined CPU.
4. **Arithmetic**: Booth's algorithm, add-and-shift multiplication, restoring division and hex ↔ BCD, run bit by bit. Operands can come from the last IMUL or IDIV.
5. **Data Lab**: hashing (linear, quadratic, chaining) on the compiler's own symbols, sorting the profiler's instruction counts, and searching data memory.
6. **Parse & Chip**:
   - **Parse tree (2D animation)**: each statement's expression tree is built the way the compiler builds it from postfix. Operands are pushed onto the stack column, and each operator pops its operands, links them as children and is pushed back, with nodes gliding along curved paths into the tree. Code generation then walks the tree in post-order: a value dot travels up each edge, every node shows its register, and a connector points to the x86-64 lines it produced.
   - **3D Chip**: the datapath as a lit, texture-mapped processor die, with an animated **3D ↔ 2D** switch that flattens perspective into an orthographic top view.

---

## How every Course Outcome is used

Every CO folder holds code that the simulator actually runs, plus a README and an explanation PDF (statement, where it shows in the app, files and functions, how it works, worked example, viva questions).

### PL (2310214L): Data Structures — C++

| CO | What it does in the simulator | Folder |
|---|---|---|
| **CO1** | The CPU's register file is an array: 16 registers addressed by number, so reading or writing RAX, RBX ... R15 is one O(1) index operation. | [`CO1_Arrays`](src/PL_2310214L/CO1_Arrays/) · [PDF](src/PL_2310214L/CO1_Arrays/CO1_Explanation.pdf) |
| **CO2** | Three different linked lists, each picked for its shape: singly for the lexer's token stream, doubly for the Step Back history, circular for the FETCH-DECODE-EXECUTE-MEMORY-WRITEBACK cycle. | [`CO2_LinkedLists`](src/PL_2310214L/CO2_LinkedLists/) · [PDF](src/PL_2310214L/CO2_LinkedLists/CO2_Explanation.pdf) |
| **CO3** | The compiler turns infix expressions into postfix with the shunting-yard algorithm, which needs an operator stack and an output queue; code generation then uses a value stack and a stack of free registers. The waveform is a circular queue. | [`CO3_StackQueue`](src/PL_2310214L/CO3_StackQueue/) · [PDF](src/PL_2310214L/CO3_StackQueue/CO3_Explanation.pdf) |
| **CO4** | Two problems in the simulator are solved with sorting and searching. (1) Turning a mnemonic into an instruction: the opcode table is quick-sorted once and every mnemonic is found by binary search; the entry found carries the function that builds the instruction, so this is the only way text becomes an instruction. (2) Showing memory: memory is a hash map with no order, so the Data Memory panel quick-sorts the used words by address and names each address by binary search in the symbols. The Data Lab also animates bubble, insertion and quick sort and linear, binary and Fibonacci search. | [`CO4_SearchingSorting`](src/PL_2310214L/CO4_SearchingSorting/) · [PDF](src/PL_2310214L/CO4_SearchingSorting/CO4_Explanation.pdf) |
| **CO5** | Variable names (compiler symbol table) and labels (assembler) are stored in a hash table whose collision strategy the user chooses: linear probing, quadratic probing or separate chaining. A generic chained HashMap provides O(1) lookups everywhere else. | [`CO5_Hashing`](src/PL_2310214L/CO5_Hashing/) · [PDF](src/PL_2310214L/CO5_Hashing/CO5_Explanation.pdf) |

### COA (2310221L): Computer Organization & Architecture — NASM x86-64 assembly

| CO | What it does in the simulator | Folder |
|---|---|---|
| **CO1** | The simulator's ALU is written in x86-64 assembly and runs on the real processor's ALU: the simulated flags are loaded into RFLAGS, the real ADD / SUB / IMUL / AND / SHL ... runs, and the processor's own CF ZF SF OF are read back. | [`CO1_Architecture`](src/COA_2310221L/CO1_Architecture/) · [PDF](src/COA_2310221L/CO1_Architecture/CO1_Explanation.pdf) |
| **CO2** | Programs written in assembly (or generated by the compiler) are assembled by a two-pass assembler and run on the simulated CPU. The instruction semantics that depend on processor details are themselves assembly: branch conditions use the real SETcc, IDIV the real IDIV, CQO the real CQO. | [`CO2_AssemblyISA`](src/COA_2310221L/CO2_AssemblyISA/) · [PDF](src/COA_2310221L/CO2_AssemblyISA/CO2_Explanation.pdf) |
| **CO3** | Booth's multiplication, add-and-shift multiplication, restoring division and binary / BCD conversion are written in assembly on n-bit registers. Each records every step (A, Q, Q-1 or C, operation) into an array that the Arithmetic screen animates bit by bit. | [`CO3_ComputerArithmetic`](src/COA_2310221L/CO3_ComputerArithmetic/) · [PDF](src/COA_2310221L/CO3_ComputerArithmetic/CO3_Explanation.pdf) |
| **CO4** | The hardwired control unit's decoder ROM is in assembly: one 32-bit word per instruction form holding the 11 control signals and the ALU operation. The 5-stage pipeline's timing rules (operand readiness with and without forwarding, and stage scheduling with stalls) are also assembly functions. | [`CO4_ControlUnitPipeline`](src/COA_2310221L/CO4_ControlUnitPipeline/) · [PDF](src/COA_2310221L/CO4_ControlUnitPipeline/CO4_Explanation.pdf) |

### CG (2310218L): Computer Graphics — C++ / OpenGL

| CO | What it does in the simulator | Folder |
|---|---|---|
| **CO1** | The whole user interface (cards, buttons, wires, waveform, LEDs, text) is drawn with OpenGL primitives; no UI library is used. | [`CO1_Primitives`](src/CG_2310218L/CO1_Primitives/) · [PDF](src/CG_2310218L/CO1_Primitives/CO1_Explanation.pdf) |
| **CO2** | The datapath diagram can be zoomed and panned in real time. Its view is a 3 × 3 homogeneous matrix M = T(centre + pan) · S(zoom) · T(-world centre); its inverse maps the mouse back so the zoom stays under the cursor. The 3D views use the OpenGL matrix stack (glTranslatef, glRotatef, glScalef). | [`CO2_Transformations`](src/CG_2310218L/CO2_Transformations/) · [PDF](src/CG_2310218L/CO2_Transformations/CO2_Explanation.pdf) |
| **CO3** | Bresenham's line algorithm turns active wires into flowing signal dots, the midpoint circle algorithm draws the flag LEDs and wire junctions, the scan-line algorithm fills the ALU and multiplexer symbols, and extrusion turns the same 2D outlines into the 3D chip. | [`CO3_ObjectAlgorithms`](src/CG_2310218L/CO3_ObjectAlgorithms/) · [PDF](src/CG_2310218L/CO3_ObjectAlgorithms/CO3_Explanation.pdf) |
| **CO4** | Every 3D view sets up a perspective camera with gluPerspective and gluLookAt. The 3D chip has an orbit camera and an animated 3D ↔ 2D switch: a dolly zoom narrows the field of view while the eye backs away until the projection becomes parallel (glOrtho top view). gluProject maps 3D points back to the screen for crisp labels. | [`CO4_ProjectionViewing`](src/CG_2310218L/CO4_ProjectionViewing/) · [PDF](src/CG_2310218L/CO4_ProjectionViewing/CO4_Explanation.pdf) |
| **CO5** | The 2D interface is clipped entirely in software: it does not use glScissor. Every card, list, button, wire, glyph and icon passes through a clipping stage before it reaches OpenGL: Sutherland-Hodgman clips every filled shape (keeping its colours and texture coordinates), Cohen-Sutherland outcodes classify every line, Liang-Barsky gives dashed lines and the waveform traces their visible part, and points are clipped by an inside test. | [`CO5_Clipping`](src/CG_2310218L/CO5_Clipping/) · [PDF](src/CG_2310218L/CO5_Clipping/CO5_Explanation.pdf) |
| **CO6** | All text is a TrueType glyph atlas texture; the 3D chip's board is a procedurally generated circuit texture with mipmaps; the chip is lit by two lights (one tinted with the current pipeline phase colour) with specular highlights, emissive glow on the active unit, and depth fog. | [`CO6_TextureLightColor`](src/CG_2310218L/CO6_TextureLightColor/) · [PDF](src/CG_2310218L/CO6_TextureLightColor/CO6_Explanation.pdf) |

### PSOOP (2310261L): Problem Solving with OOP — C++

| CO | What it does in the simulator | Folder |
|---|---|---|
| **CO1** | The simulator models a real computer as classes (CPU, ALU, Register64, Assembler, Compiler, ControlUnit, Pipeline) and solves a real problem for beginners: understanding code written in plain English. The EnglishTranslator turns sentences like “add 12 and 3” into program statements. | [`CO1_RealWorldOOP`](src/PSOOP_2310261L/CO1_RealWorldOOP/) · [PDF](src/PSOOP_2310261L/CO1_RealWorldOOP/CO1_Explanation.pdf) |
| **CO2** | Complexity is reduced with abstraction and inheritance: the CPU only calls execute() on an abstract Instruction; control units, hash tables, sorters and screens are abstract base classes with interchangeable subclasses; containers are templates; and every error is one typed exception hierarchy caught in one place. | [`CO2_ReduceComplexity`](src/PSOOP_2310261L/CO2_ReduceComplexity/) · [PDF](src/PSOOP_2310261L/CO2_ReduceComplexity/CO2_Explanation.pdf) |
| **CO3** | The central problem of the project: compile statements and simple C++ into x86-64 assembly. The compiler splits it into three stages (lexer, shunting-yard parser, code generator), each recorded so the screen can animate it. | [`CO3_ProblemSolving`](src/PSOOP_2310261L/CO3_ProblemSolving/) · [PDF](src/PSOOP_2310261L/CO3_ProblemSolving/CO3_Explanation.pdf) |
| **CO4** | Design patterns and C++ features each solve a concrete problem in the simulator: the Factory builds instructions, the Observer profiles execution, the Settings singleton keeps one set of preferences in settings.cfg, Strategies make the control unit, hash method and sort interchangeable, Command / undo powers Step Back, the friend class Debugger lets the user edit a register without exposing a public setter, and static members count the instruction mix. | [`CO4_LogicBuilding`](src/PSOOP_2310261L/CO4_LogicBuilding/) · [PDF](src/PSOOP_2310261L/CO4_LogicBuilding/CO4_Explanation.pdf) |
## Project structure — organised by subject and course outcome

```
src/
├── PL_2310214L/                 Data structures (C++)
│   ├── CO1_Arrays/              FixedArray<T,N>                     -> CPU register file
│   ├── CO2_LinkedLists/         Singly / Doubly / CircularLinkedList -> tokens, Step Back, phase ring
│   ├── CO3_StackQueue/          Stack<T>, CircularQueue<T>          -> shunting-yard, code gen, waveform
│   ├── CO4_SearchingSorting/    Sorting, Searching, TableSort, TableSearch -> opcode lookup, memory view, Data Lab
│   └── CO5_Hashing/             HashTable, OpenAddressing, Chaining, HashMap -> symbol / label tables
├── COA_2310221L/                Computer organization (NASM x86-64, linked into the app)
│   ├── CO1_Architecture/        alu.asm            the ALU on the real x86 ALU + RFLAGS
│   ├── CO2_AssemblyISA/         isa.asm            Jcc conditions (SETcc), IDIV, CQO
│   ├── CO3_ComputerArithmetic/  arithmetic.asm     Booth, add-and-shift, restoring division, BCD
│   ├── CO4_ControlUnitPipeline/ control_unit.asm   decoder ROM, operand readiness, pipeline schedule
│   ├── coa_asm.h                C++ declarations of the assembly functions
│   └── abi.inc                  Linux / Windows calling conventions
├── CG_2310218L/                 Computer graphics (C++ / OpenGL)
│   ├── CO1_Primitives/          Draw               every UI shape from GL primitives
│   ├── CO2_Transformations/     Math               Mat3 translate / scale / rotate, pan & zoom
│   ├── CO3_ObjectAlgorithms/    Raster, Extrude    Bresenham, midpoint circle, scan-line, 2D -> 3D
│   ├── CO4_ProjectionViewing/   Scene3D, Camera    gluPerspective / glOrtho, gluLookAt, dolly zoom
│   ├── CO5_Clipping/            Clip, PolygonClip, Clipper  software clipping of the whole 2D UI
│   └── CO6_TextureLightColor/   Font, Texture, Lighting  glyph atlas, PCB texture + mipmaps, lights, fog
├── PSOOP_2310261L/              Object-oriented core (C++)
│   ├── CO1_RealWorldOOP/        English            English -> statements translator
│   ├── CO2_ReduceComplexity/    Errors             exception hierarchy
│   ├── CO3_ProblemSolving/      Compiler           lexer, shunting-yard, code generator
│   └── CO4_LogicBuilding/       Debugger, Profiler, Clock, Settings  friend, Observer, Singletons
├── core/                        the CPU model: Register64, CPU, instructions, assembler, control unit, pipeline
└── app/                         window, simulation controller, widgets, the six screens
tests/       console unit tests (131 checks)
docs/        make_co_pdfs.py  -> the CO explanation PDFs
assets/      fonts (Lato, Noto Sans Mono, DejaVu Sans)
third_party/ stb_truetype.h
```
