// =====================================================================
//  Computer arithmetic algorithms                         [COA CO3]
//  Step-by-step register traces of the algorithms the ALU hardware
//  uses, so the Arithmetic view can animate them bit by bit:
//    * Booth's algorithm      - signed multiplication
//    * Add-and-shift          - unsigned multiplication
//    * Restoring division     - unsigned division
//    * Number systems         - binary / hex / BCD views of a register
// =====================================================================
#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace vas::core {

struct ArithStep {
    std::string cycle;
    std::vector<std::string> regs;   // one bit-string per register column
    std::string action;
    bool isShift = false;
};

struct ArithTrace {
    std::string title;
    std::string summary;
    std::vector<std::string> columns;   // register names, e.g. A Q Q-1 M
    std::vector<ArithStep> steps;
    std::string result;
    bool ok = true;
    int bits = 8;
};

ArithTrace boothMultiply(int multiplicand, int multiplier, int bits = 8);
ArithTrace addShiftMultiply(unsigned multiplicand, unsigned multiplier, int bits = 8);
ArithTrace restoringDivide(unsigned dividend, unsigned divisor, int bits = 8);

struct NumberViews {
    std::string dec, hex, bin, bcd, bcdBack;
    bool bcdOk = true;
};
NumberViews numberViews(std::int64_t v);

std::string toBits(std::uint64_t v, int bits);

} // namespace vas::core
