// =====================================================================
//  Register64, Flags and the ALU                     [COA CO1]
//  Register64 overloads the arithmetic / logic / shift operators so
//  the ALU code reads like the hardware equations.           [PSOOP]
//  Sub-register views (EAX / AX / AH / AL) show the 8086 -> 80386 ->
//  x86-64 lineage of the register file.
// =====================================================================
#pragma once
#include <cstdint>
#include <ostream>
#include <string>

#include "engine/asm/coa_asm.h"

namespace vas::core {

class Register64 {
public:
    Register64(std::uint64_t v = 0) : v_(v) {}

    std::uint64_t u() const { return v_; }
    std::int64_t s() const { return static_cast<std::int64_t>(v_); }

    // 80386 / 8086 sub-registers of RAX-style registers
    std::uint32_t e32() const { return static_cast<std::uint32_t>(v_); }
    std::uint16_t x16() const { return static_cast<std::uint16_t>(v_); }
    std::uint8_t h8() const { return static_cast<std::uint8_t>(v_ >> 8); }
    std::uint8_t l8() const { return static_cast<std::uint8_t>(v_); }
    bool msb() const { return (v_ >> 63) & 1; }

    Register64 operator+(const Register64& o) const { return Register64(v_ + o.v_); }
    Register64 operator-(const Register64& o) const { return Register64(v_ - o.v_); }
    Register64 operator*(const Register64& o) const { return Register64(v_ * o.v_); }
    Register64 operator&(const Register64& o) const { return Register64(v_ & o.v_); }
    Register64 operator|(const Register64& o) const { return Register64(v_ | o.v_); }
    Register64 operator^(const Register64& o) const { return Register64(v_ ^ o.v_); }
    Register64 operator~() const { return Register64(~v_); }
    Register64 operator-() const { return Register64(0 - v_); }
    Register64 operator<<(unsigned n) const { return Register64(n >= 64 ? 0 : v_ << n); }
    Register64 operator>>(unsigned n) const { return Register64(n >= 64 ? 0 : v_ >> n); }   // logical
    bool operator==(const Register64& o) const { return v_ == o.v_; }
    bool operator!=(const Register64& o) const { return v_ != o.v_; }
    bool operator<(const Register64& o) const { return v_ < o.v_; }                        // unsigned

    std::string hex(bool grouped = true) const {
        static const char* d = "0123456789ABCDEF";
        std::string s;
        for (int i = 15; i >= 0; --i) {
            s += d[(v_ >> (i * 4)) & 0xF];
            if (grouped && i && i % 4 == 0) s += '_';
        }
        return s;
    }

    friend std::ostream& operator<<(std::ostream& os, const Register64& r) { return os << "0x" << r.hex(false); }

private:
    std::uint64_t v_;
};

struct Flags {
    bool CF = false;   // carry      (bit 0)
    bool ZF = false;   // zero       (bit 6)
    bool SF = false;   // sign       (bit 7)
    bool OF = false;   // overflow   (bit 11)

    bool operator==(const Flags& o) const { return CF == o.CF && ZF == o.ZF && SF == o.SF && OF == o.OF; }
    bool operator!=(const Flags& o) const { return !(*this == o); }
    std::uint64_t rflags() const { return 0x2 | (CF ? 1u : 0u) | (ZF ? 0x40u : 0u) | (SF ? 0x80u : 0u) | (OF ? 0x800u : 0u); }
    static Flags fromRflags(std::uint64_t r) {
        Flags f;
        f.CF = r & 0x1; f.ZF = r & 0x40; f.SF = r & 0x80; f.OF = r & 0x800;
        return f;
    }
};

enum class AluOp { None, Pass, Add, Sub, Mul, Div, And, Or, Xor, Not, Neg, Shl, Shr, Inc, Dec };

inline const char* aluOpName(AluOp op) {
    switch (op) {
        case AluOp::None: return "---";
        case AluOp::Pass: return "PASS";
        case AluOp::Add: return "ADD";
        case AluOp::Sub: return "SUB";
        case AluOp::Mul: return "MUL";
        case AluOp::Div: return "DIV";
        case AluOp::And: return "AND";
        case AluOp::Or: return "OR";
        case AluOp::Xor: return "XOR";
        case AluOp::Not: return "NOT";
        case AluOp::Neg: return "NEG";
        case AluOp::Shl: return "SHL";
        case AluOp::Shr: return "SHR";
        case AluOp::Inc: return "INC";
        case AluOp::Dec: return "DEC";
    }
    return "?";
}

inline const char* aluOpSymbol(AluOp op) {
    switch (op) {
        case AluOp::Add: case AluOp::Inc: return "+";
        case AluOp::Sub: case AluOp::Dec: case AluOp::Neg: return "-";
        case AluOp::Mul: return "x";
        case AluOp::Div: return "/";
        case AluOp::And: return "&";
        case AluOp::Or: return "|";
        case AluOp::Xor: return "^";
        case AluOp::Not: return "~";
        case AluOp::Shl: return "<<";
        case AluOp::Shr: return ">>";
        default: return "";
    }
}

struct AluResult {
    Register64 value;
    Flags flags;
};

// Combinational ALU: result + status flags. The work is done by
// vas_alu (engine/asm/CO1_Architecture/alu.asm), which runs the real x86
// instruction and reads back the processor's own flags.
class ALU {
public:
    static AluResult compute(AluOp op, Register64 a, Register64 b, const Flags& in) {
        std::uint64_t rflags = in.rflags();
        std::uint64_t v = vas_alu(static_cast<int>(op), a.u(), b.u(), &rflags);
        return {Register64(v), Flags::fromRflags(rflags)};
    }
};

} // namespace vas::core
