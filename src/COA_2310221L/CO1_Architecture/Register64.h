// =====================================================================
//  Register64, Flags and the ALU                     [COA CO1, CO3]
//  Register64 overloads the arithmetic / logic / shift operators so
//  the ALU code reads like the hardware equations.           [PSOOP]
//  Sub-register views (EAX / AX / AH / AL) show the 8086 -> 80386 ->
//  x86-64 lineage of the register file.
// =====================================================================
#pragma once
#include <cstdint>
#include <ostream>
#include <string>

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

// Combinational ALU: result + status flags, exactly like x86-64.
class ALU {
public:
    static AluResult compute(AluOp op, Register64 a, Register64 b, const Flags& in) {
        AluResult r{a, in};
        Flags& f = r.flags;
        auto zs = [&](Register64 v) { f.ZF = v.u() == 0; f.SF = v.msb(); };
        switch (op) {
            case AluOp::None:
                break;
            case AluOp::Pass:
                r.value = b;
                break;
            case AluOp::Add: {
                r.value = a + b;
                f.CF = r.value < a;
                f.OF = (((a ^ r.value) & (b ^ r.value)).msb());
                zs(r.value);
                break;
            }
            case AluOp::Sub: {
                r.value = a - b;
                f.CF = a < b;
                f.OF = (((a ^ b) & (a ^ r.value)).msb());
                zs(r.value);
                break;
            }
            case AluOp::Inc: {
                r.value = a + Register64(1);
                f.OF = a.u() == 0x7FFFFFFFFFFFFFFFULL;
                zs(r.value);
                break;
            }
            case AluOp::Dec: {
                r.value = a - Register64(1);
                f.OF = a.u() == 0x8000000000000000ULL;
                zs(r.value);
                break;
            }
            case AluOp::Mul: {
                __int128 full = static_cast<__int128>(a.s()) * static_cast<__int128>(b.s());
                r.value = Register64(static_cast<std::uint64_t>(full));
                f.CF = f.OF = full != static_cast<__int128>(r.value.s());
                zs(r.value);
                break;
            }
            case AluOp::Div: {                       // signed quotient (b != 0 checked by CPU)
                if (b.u() == 0) break;
                if (a.s() == INT64_MIN && b.s() == -1) r.value = a;
                else r.value = Register64(static_cast<std::uint64_t>(a.s() / b.s()));
                break;
            }
            case AluOp::And: r.value = a & b; f.CF = f.OF = false; zs(r.value); break;
            case AluOp::Or:  r.value = a | b; f.CF = f.OF = false; zs(r.value); break;
            case AluOp::Xor: r.value = a ^ b; f.CF = f.OF = false; zs(r.value); break;
            case AluOp::Not: r.value = ~a; break;
            case AluOp::Neg: {
                r.value = -a;
                f.CF = a.u() != 0;
                f.OF = a.u() == 0x8000000000000000ULL;
                zs(r.value);
                break;
            }
            case AluOp::Shl: {
                unsigned n = static_cast<unsigned>(b.u() & 63);
                if (!n) break;
                r.value = a << n;
                f.CF = ((a >> (64 - n)).u() & 1);
                if (n == 1) f.OF = r.value.msb() != f.CF;
                zs(r.value);
                break;
            }
            case AluOp::Shr: {
                unsigned n = static_cast<unsigned>(b.u() & 63);
                if (!n) break;
                r.value = a >> n;
                f.CF = ((a >> (n - 1)).u() & 1);
                if (n == 1) f.OF = a.msb();
                zs(r.value);
                break;
            }
        }
        return r;
    }
};

} // namespace vas::core
