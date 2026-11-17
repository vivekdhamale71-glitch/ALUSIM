# COA — Computer Organization & Architecture · 2310221L

The processor-level parts of the simulator are written in NASM x86-64 assembly and linked into the program: the ALU (CO1), instruction semantics (CO2), the arithmetic algorithms (CO3) and the control unit's decoder plus the pipeline timing (CO4). `coa_asm.h` declares them for C++; `abi.inc` adapts them to the Linux and Windows calling conventions. The CPU model that calls them is in `src/core/`.

| CO | Folder | Statement | Explanation |
|---|---|---|---|
| 2310221L.CO1 | [`CO1_Architecture/`](CO1_Architecture/) | Explicate the architecture and instruction set of the 80386 microprocessor. | [PDF](CO1_Architecture/CO1_Explanation.pdf) |
| 2310221L.CO2 | [`CO2_AssemblyISA/`](CO2_AssemblyISA/) | Develop assembly language programs using 80386 instruction set. | [PDF](CO2_AssemblyISA/CO2_Explanation.pdf) |
| 2310221L.CO3 | [`CO3_ComputerArithmetic/`](CO3_ComputerArithmetic/) | Illustrate processor level details about arithmetic operations using computer arithmetic algorithms. | [PDF](CO3_ComputerArithmetic/CO3_Explanation.pdf) |
| 2310221L.CO4 | [`CO4_ControlUnitPipeline/`](CO4_ControlUnitPipeline/) | Demonstrate the different ways to generate control signals and their organization techniques and instruction pipelining of computer system. | [PDF](CO4_ControlUnitPipeline/CO4_Explanation.pdf) |
