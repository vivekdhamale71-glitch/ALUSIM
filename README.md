# Virtual ALU Simulator

**Write plain English or C++, watch it turn into x86-64 assembly, and see the CPU run it.** For example, `add 12 and 3`, `if marks is at least 40 then set pass to 1 otherwise set pass to 0`, `find the factorial of 5`, or `for (int i = 0; i < n; i++) sum += i;`. The simulator animates every step: the translation, the parse tree, the datapath, the control signals, the pipeline, and a 3D chip.

Second Year B.Tech (Software Engineering) mini project. It integrates **PL (2310214L)**, **COAL (2310221L)**, **CGL (2310218L)** and **PSOOP (2310261L)**.

Team: Vivek Dhamale · Pranjal Bhandari · Sanika Patil · Mrudula Bongulwar

---

## Run it

```bash
sudo apt install g++ freeglut3-dev libglu1-mesa-dev   # once
./build.sh run                                        # build + start
./build.sh test                                       # 110 unit tests for the core
```

There is also a `Makefile` (`make run`, `make test`). On Windows (MSYS2 MinGW64), install `mingw-w64-x86_64-freeglut` and run `make OS=Windows_NT`.

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

### PL (2310214L): Data Structures

| CO | Where it is used | Files |
|---|---|---|
| **CO1** Arrays | Register file `Register64[16]`, opcode table, waveform columns, bucket arrays of the hash tables | `COA_2310221L/CO1_Architecture/CPU.h`, `PL_2310214L/CO5_Hashing/HashTable.h` |
| **CO2** Linked lists | **Singly**: the lexer's token stream. **Doubly**: the execution timeline of CPU snapshots, used by Step Back. **Circular**: the FETCH→DECODE→EXECUTE→MEM→WB instruction cycle. Separate chaining also uses linked nodes. | `PL_2310214L/CO2_LinkedLists/LinkedList.h`, `PSOOP_2310261L/CO3_ProblemSolving/Compiler.cpp`, `app/Sim.h` |
| **CO3** Stack & queue | Shunting-yard operator **stack** and output **queue**, register-allocation stack, code-gen value stack, PUSH/POP/CALL/RET on the CPU stack, and a circular queue as the waveform's rolling buffer | `PL_2310214L/CO3_StackQueue/Stack.h`, `PL_2310214L/CO3_StackQueue/Queue.h`, `PSOOP_2310261L/CO3_ProblemSolving/Compiler.cpp` |
| **CO4** Searching & sorting | **Binary search** for keywords and for mnemonics in the opcode table, which is **quick-sorted** at start-up. The Data Lab runs Bubble, Insertion and Quick sort on the profiler data, and Linear, Binary and Fibonacci search on memory. | `PL_2310214L/CO4_SearchingSorting/Sorting.h`, `PL_2310214L/CO4_SearchingSorting/Searching.h`, `COA_2310221L/CO2_AssemblyISA/Instruction.cpp` |
| **CO5** Hashing | Variable symbol table and assembler label table (linear, quadratic or chaining, chosen by the user), `HashMap<K,V>` for opcode dispatch, register names, sparse memory, the micro-code dispatch ROM and the translation cache | `PL_2310214L/CO5_Hashing/HashTable.h`, `PL_2310214L/CO5_Hashing/HashMap.h` |

### COAL (2310221L): Computer Organization & Architecture

| CO | Where it is used | Files |
|---|---|---|
| **CO1** Architecture & instruction set | 16 × 64-bit registers with **RAX ⊃ EAX ⊃ AX ⊃ AH/AL** sub-register views (8086 → 80386 → x86-64), RIP, RFLAGS (CF ZF SF OF), memory map and stack | `COA_2310221L/CO1_Architecture/Register64.h`, `COA_2310221L/CO1_Architecture/CPU.cpp`, register panel |
| **CO2** Assembly programs | 33 real x86 instructions (MOV, ADD, SUB, IMUL, IDIV, CQO, logic, shifts, CMP, Jcc, CALL/RET, PUSH/POP…), a two-pass assembler, and programs written by hand or generated by the compiler | `COA_2310221L/CO2_AssemblyISA/Assembler.cpp`, `COA_2310221L/CO2_AssemblyISA/Instruction.cpp` |
| **CO3** Computer arithmetic | Booth's algorithm, add-and-shift, restoring division and BCD conversion, step by step. The ALU computes CF, ZF, SF and OF exactly as x86 does. | `COA_2310221L/CO3_ComputerArithmetic/Arithmetic.cpp`, `COA_2310221L/CO1_Architecture/Register64.h` |
| **CO4** Control signals & pipelining | **Hardwired** (decoder table) vs **Microprogrammed** (41-word control store, micro-PC, dispatch ROM) control units, an 11-signal control vector per cycle, a waveform, and a **5-stage pipeline** with data, control and structural hazards, forwarding, CPI and speed-up | `COA_2310221L/CO4_ControlUnitPipeline/ControlUnit.cpp`, `COA_2310221L/CO4_ControlUnitPipeline/Pipeline.cpp` |

### CGL (2310218L): Computer Graphics (OpenGL)

Every technique has a job in the interface. None of them is a separate demo.

| CO | Where it is used | Files |
|---|---|---|
| **CO1** OpenGL primitives | The whole UI (cards, buttons, wires, waveform, glyphs) is built from `GL_TRIANGLE_FAN`, `GL_TRIANGLE_STRIP`, `GL_QUADS`, `GL_POINTS` and `GL_LINES` | `CG_2310218L/CO1_Primitives/Draw.cpp` |
| **CO2** Transformations | `Mat3` translate and scale for datapath pan and zoom (zoom about the cursor), translation of packets, tokens, swapped bars and shifting bits, and rotation of the 3D camera | `CG_2310218L/CO2_Transformations/Math.h`, `app/DatapathView.cpp` |
| **CO3** 2D & 3D object algorithms | **Bresenham** draws the active wires as flowing signal dots, the **midpoint circle** draws the flag LEDs, the **scanline fill** fills the concave ALU and MUX symbols, and 2D outlines are **extruded** into 3D prisms | `CG_2310218L/CO3_ObjectAlgorithms/Raster.cpp`, `app/ChipView.cpp` |
| **CO4** Projection & viewing | Animated 3D → 2D switch: a dolly zoom flattens `gluPerspective` into `glOrtho` for a top view of the chip. Orbit camera with `gluLookAt`. | `app/ParseTree.cpp`, `app/ChipView.cpp` |
| **CO5** Clipping | **Cohen–Sutherland** clips datapath wires when you zoom or pan. **Liang–Barsky** clips the scrolling waveform traces. | `CG_2310218L/CO5_Clipping/Clip.cpp` |
| **CO6** Texture, light, colour | TrueType glyph atlas texture, a procedurally generated PCB texture with mipmaps, two lights with specular highlights, emissive glow on the active unit, fog, and phase colour-coding | `CG_2310218L/CO6_TextureLightColor/Font.cpp`, `app/ChipView.cpp` |

### PSOOP (2310261L): Problem Solving with OOP (C++)

| CO | Where it is used |
|---|---|
| **CO1** OOP for real problems | The CPU, ALU, Register64, Compiler, Assembler, ControlUnit, Pipeline and each View are classes that model real hardware and software parts. |
| **CO2** Reducing complexity | Abstract `Instruction` with 6 derived classes (the CPU only calls `execute()`), abstract `ControlUnit`, `HashTable`, `Sorter`, `Searcher` and `View`, templates (`Stack<T>`, `CircularQueue<T>`, `HashMap<K,V>`, `quickSort`, `binarySearch`), and namespaces |
| **CO3** Critical thinking & programming | The English → code translator (a recursive-descent parser over words), the C++ front end (declarations, compound operators, for-loops), an exception hierarchy (`SyntaxError`, `DivideByZeroError`, `MemoryAccessError`, `StackOverflowError`…), operator overloading (`Register64`, `Vec2`, `Mat3`, `Flags`), and rule-of-five containers |
| **CO4** Logic building | Design patterns: **Factory** (instructions from a mnemonic), **Observer** (Profiler watching the CPU), **Singleton** (Clock), **Strategy** (control units, hash and sort methods), **Command and undo** (Step Back), plus a **friend** class (Debugger) and a static member (instruction counter). File handling for save, load and trace export. |

---

## Project structure — organised by subject and course outcome

Every subject has its own folder and every CO has its own sub-folder with a README
(official CO statement, the code in it, where it shows up in the app).

```
src/
├── PL_2310214L/                 Programming Lab — Data Structures
│   ├── CO1_Arrays/              FixedArray<T,N>  (CPU register file)
│   ├── CO2_LinkedLists/         singly / doubly / circular linked lists
│   ├── CO3_StackQueue/          Stack<T>, CircularQueue<T>
│   ├── CO4_SearchingSorting/    bubble / insertion / quick sort, linear / binary / Fibonacci search
│   └── CO5_Hashing/             linear & quadratic probing, chaining, HashMap<K,V>
├── COA_2310221L/                Computer Organization & Architecture
│   ├── CO1_Architecture/        Register64 + ALU + flags, CPU
│   ├── CO2_AssemblyISA/         33-instruction x86 ISA, two-pass assembler
│   ├── CO3_ComputerArithmetic/  Booth, add-and-shift, restoring division, BCD
│   └── CO4_ControlUnitPipeline/ hardwired & microprogrammed control unit, 5-stage pipeline
├── CG_2310218L/                 Computer Graphics (OpenGL)
│   ├── CO1_Primitives/          OpenGL drawing layer
│   ├── CO2_Transformations/     Vec2, Mat3 (translate / scale / rotate)
│   ├── CO3_ObjectAlgorithms/    Bresenham, midpoint circle, scanline fill
│   ├── CO4_ProjectionViewing/   3D scenes: perspective camera, gluProject
│   ├── CO5_Clipping/            Cohen–Sutherland, Liang–Barsky
│   └── CO6_TextureLightColor/   TrueType texture atlas (+ lighting / fog in the 3D chip)
├── PSOOP_2310261L/              Problem Solving using OOP
│   ├── CO1_RealWorldOOP/        English → code translator
│   ├── CO2_ReduceComplexity/    exception hierarchy
│   ├── CO3_ProblemSolving/      C++ / statement → x86-64 compiler
│   └── CO4_LogicBuilding/       Observer, Singleton, friend Debugger
└── app/                         window, simulation controller, widgets, the six screens
tests/       console unit tests (110 checks)
assets/      fonts (Lato, Noto Sans Mono, DejaVu Sans)
third_party/ stb_truetype.h
```
# virtualalu
