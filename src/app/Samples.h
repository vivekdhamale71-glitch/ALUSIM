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
};

inline const std::vector<Sample>& samples() {
    static const std::vector<Sample> s = {
        // ------------------------------------------------------------ English
        {"Add two numbers", "add 12 and 3\n", MODE_ENGLISH},
        {"Arithmetic in words",
         "set price to 250\nset quantity to 4\nmultiply price by quantity and store it in total\n"
         "subtract 100 from total\n",
         MODE_ENGLISH},
        {"Precedence: plus and times", "compute 2 plus 3 times 4 and store it in answer\n", MODE_ENGLISH},
        {"Greatest of three numbers", "set a to 9\nset b to 4\nset c to 7\nfind the largest of a and b and c\n",
         MODE_ENGLISH},
        {"If / otherwise",
         "set marks to 72\nif marks is at least 40 then set pass to 1 otherwise set pass to 0\n", MODE_ENGLISH},
        {"Factorial", "find the factorial of 5\n", MODE_ENGLISH},
        {"Sum of 1 to n", "find the sum of numbers from 1 to 10\n", MODE_ENGLISH},
        {"Repeat a step", "set count to 0\nrepeat 5 times: increase count by 3\n", MODE_ENGLISH},
        {"Multiply by repeated addition", "multiply 6 by 7 using repeated addition\n", MODE_ENGLISH},
        {"Power and remainder",
         "raise 2 to the power of 8\nfind the remainder of 100 divided by 7 and store it in r\n", MODE_ENGLISH},
        {"Swap two variables", "set x to 3\nset y to 8\nswap x and y\n", MODE_ENGLISH},
        // ---------------------------------------------------------------- C++
        {"C++: expressions",
         "#include <iostream>\nusing namespace std;\n\nint main() {\n    int a = 5, b = 7;\n    int c = a + b * 2;\n"
         "    a += 10;\n    b--;\n    cout << c << endl;\n    return 0;\n}\n",
         MODE_CPP},
        {"C++: for loop factorial",
         "int main() {\n    long fact = 1;\n    for (int n = 5; n > 1; n--) {\n        fact *= n;\n    }\n"
         "    return 0;\n}\n",
         MODE_CPP},
        {"C++: greatest of three",
         "int main() {\n    int a = 9, b = 4, c = 7;\n    int max = a;\n    if (b > max) max = b;\n"
         "    if (c > max) max = c;\n    return 0;\n}\n",
         MODE_CPP},
        {"C++: while loop sum",
         "int main() {\n    int n = 10, sum = 0;\n    while (n > 0) {\n        sum += n;\n        n--;\n    }\n"
         "    return 0;\n}\n",
         MODE_CPP},
        {"C++: bit tricks",
         "int main() {\n    int v = 0x2C;\n    int low = v & 15;\n    int high = v >> 4;\n    v <<= 1;\n"
         "    v ^= 255;\n    return 0;\n}\n",
         MODE_CPP},
        // ----------------------------------------------------------- assembly
        {"ASM: loop + CALL / RET",
         "section .data\n    n    dq 5\n    sum  dq 0\n\nsection .text\n_start:\n    mov rcx, [n]\n    xor rax, rax\n"
         "loop:\n    add rax, rcx\n    dec rcx\n    jnz loop\n    mov [sum], rax\n    push rax\n    call double\n"
         "    pop rbx\n    hlt\n\ndouble:\n    shl rax, 1\n    ret\n",
         MODE_ASM},
        {"ASM: overflow & carry flags",
         "section .text\n_start:\n    mov rax, 0x7FFFFFFFFFFFFFFF\n    add rax, 1        ; OF=1 SF=1\n"
         "    mov rbx, 3\n    sub rbx, 5        ; CF=1 (borrow)\n    mov rcx, 1\n    shl rcx, 63       ; SF=1\n"
         "    xor rdx, rdx      ; ZF=1\n    hlt\n",
         MODE_ASM},
    };
    return s;
}

} // namespace vas::ui
