// Built-in example programs: English sentences, C++ and hand-written assembly.
#pragma once
#include <string>
#include <vector>

namespace vas::ui {

enum InputMode { MODE_ENGLISH = 0, MODE_CPP = 1, MODE_ASM = 2 };

struct Sample {
    std::string name;
    std::string code;
    int mode;
    std::string use;   // which tab or job this example is best for
};

inline const std::vector<Sample>& samples() {
    static const std::vector<Sample> s = {
        // Every name starts with what the program is best for.
        // ------------------------------------------------------------ English
        {"Parse tree: precedence (2 + 3 * 4)", "compute 2 plus 3 times 4 and store it in answer\n", MODE_ENGLISH, "Parse & Chip tab: shows * binding tighter than +"},
        {"Compiler: arithmetic in words",
         "set price to 250\nset quantity to 4\nmultiply price by quantity and store it in total\n"
         "subtract 100 from total\n",
         MODE_ENGLISH, "Compiler tab: English to assembly, step by step"},
        {"Branching: if / otherwise",
         "set marks to 72\nif marks is at least 40 then set pass to 1 otherwise set pass to 0\n", MODE_ENGLISH, "Compiler tab: compare and jump code"},
        {"Loops: factorial of 5", "find the factorial of 5\n", MODE_ENGLISH, "Datapath tab: a loop running on the datapath"},
        // ---------------------------------------------------------------- C++
        {"Parse tree: variables and operators (C++)",
         "#include <iostream>\nusing namespace std;\n\nint main() {\n    int a = 5, b = 7;\n    int c = a + b * 2;\n"
         "    a += 10;\n    b--;\n    cout << c << endl;\n    return 0;\n}\n",
         MODE_CPP, "Parse & Chip tab: variables, += and -- as trees"},
        {"Pipeline: while loop sum (C++)",
         "int main() {\n    int n = 10, sum = 0;\n    while (n > 0) {\n        sum += n;\n        n--;\n    }\n"
         "    return 0;\n}\n",
         MODE_CPP, "Pipeline tab: a loop with data hazards"},
        {"Arithmetic: bit operations (C++)",
         "int main() {\n    int v = 0x2C;\n    int low = v & 15;\n    int high = v >> 4;\n    v <<= 1;\n"
         "    v ^= 255;\n    return 0;\n}\n",
         MODE_CPP, "Arithmetic tab: AND, shifts and XOR"},
        {"Data Lab: hash collisions (C++)",
         "// Compound interest: how much do 100 coins grow in 3 years at 5% per year?\n"
         "// Test the three hash methods on this program's variables:\n"
         "// s, rate, count, left, index and result all share the same home slot (13).\n"
         "int main() {\n"
         "    int s = 100;            // savings\n"
         "    int rate = 5;           // percent per year\n"
         "    int count = 3;          // years to wait\n"
         "    int left = count;       // years still to go\n"
         "    int index = 0;          // years passed\n"
         "    int result = s;         // balance\n"
         "    while (left > 0) {\n"
         "        result = result + result * rate / 100;   // add one year of interest\n"
         "        left--;\n"
         "        index++;\n"
         "    }\n"
         "}\n",
         MODE_CPP, "Data Lab tab: hash collisions (all share slot 13)"},
        // ----------------------------------------------------------- assembly
        {"Datapath: loop + CALL / RET (ASM)",
         "section .data\n    n    dq 5\n    sum  dq 0\n\nsection .text\n_start:\n    mov rcx, [n]\n    xor rax, rax\n"
         "loop:\n    add rax, rcx\n    dec rcx\n    jnz loop\n    mov [sum], rax\n    push rax\n    call double\n"
         "    pop rbx\n    hlt\n\ndouble:\n    shl rax, 1\n    ret\n",
         MODE_ASM, "Datapath tab: CALL / RET and the stack"},
        {"Flags: overflow and carry (ASM)",
         "section .text\n_start:\n    mov rax, 0x7FFFFFFFFFFFFFFF\n    add rax, 1        ; OF=1 SF=1\n"
         "    mov rbx, 3\n    sub rbx, 5        ; CF=1 (borrow)\n    mov rcx, 1\n    shl rcx, 63       ; SF=1\n"
         "    xor rdx, rdx      ; ZF=1\n    hlt\n",
         MODE_ASM, "Datapath tab: CF, ZF, SF and OF flags"},
    };
    return s;
}

} // namespace vas::ui
