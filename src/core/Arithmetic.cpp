#include "core/Arithmetic.h"

#include <cstdio>
#include <sstream>

#include "COA_2310221L/coa_asm.h"

namespace vas::core {

std::string toBits(std::uint64_t v, int bits) {
    std::string s;
    for (int i = bits - 1; i >= 0; --i) s += ((v >> i) & 1) ? '1' : '0';
    return s;
}

static std::int64_t signExtend(std::uint64_t v, int bits) {
    std::uint64_t m = 1ULL << (bits - 1);
    v &= (1ULL << bits) - 1;
    return std::int64_t((v ^ m) - m);
}

// The algorithms run in COA_2310221L/CO3_ComputerArithmetic/arithmetic.asm and
// leave one ArithStepRec per step; this file only turns them into text.
static std::vector<ArithStepRec> records(std::size_t n) { return std::vector<ArithStepRec>(1 + 3 * n); }

ArithTrace boothMultiply(int mIn, int qIn, int n) {
    ArithTrace t;
    t.title = "Booth's Algorithm";
    std::string widened;
    // -2^(n-1) cannot be negated in n bits (A - M would overflow): use one more bit
    if (signExtend(std::uint64_t(mIn), n) == -(std::int64_t(1) << (n - 1))) {
        widened = "  (M = " + std::to_string(-(1 << (n - 1))) + " cannot be negated in " + std::to_string(n) +
                  " bits, so the registers are widened to " + std::to_string(n + 1) + " bits)";
        mIn = int(signExtend(std::uint64_t(mIn), n));
        qIn = int(signExtend(std::uint64_t(qIn), n));
        ++n;
    }
    t.bits = n;
    t.columns = {"A", "Q", "Q-1", "M"};
    std::uint64_t mask = (1ULL << n) - 1, M = std::uint64_t(mIn) & mask;
    std::int64_t m = signExtend(M, n), q = signExtend(std::uint64_t(qIn) & mask, n);
    std::ostringstream sum;
    sum << "Signed " << m << " x " << q << " using " << n << "-bit registers. Look at Q0 Q-1: "
        << "10 -> A = A - M,  01 -> A = A + M,  then arithmetic shift right A Q Q-1." << widened;
    t.summary = sum.str();

    auto rec = records(std::size_t(n));
    int count = vas_booth(mIn, qIn, n, rec.data());
    int cycle = 0;
    for (int i = 0; i < count; ++i) {
        const ArithStepRec& r = rec[std::size_t(i)];
        std::string act;
        switch (r.kind) {
            case STEP_INIT: act = "A = 0, Q = multiplier, Q-1 = 0, M = multiplicand"; break;
            case STEP_SUB:  act = "Q0 Q-1 = 10  ->  A = A - M"; break;
            case STEP_ADD:  act = "Q0 Q-1 = 01  ->  A = A + M"; break;
            case STEP_NOP: {
                int q0 = int(rec[std::size_t(i - 1)].Q & 1), q1 = int(rec[std::size_t(i - 1)].extra);
                act = std::string("Q0 Q-1 = ") + char('0' + q0) + char('0' + q1) + "  ->  no operation";
                break;
            }
            default: act = "Arithmetic shift right  A Q Q-1"; break;
        }
        if (r.kind == STEP_SUB || r.kind == STEP_ADD || r.kind == STEP_NOP) ++cycle;
        t.steps.push_back({r.kind == STEP_INIT ? "init" : std::to_string(cycle),
                           {toBits(r.A, n), toBits(r.Q, n), std::to_string(r.extra), toBits(M, n)}, act,
                           r.kind == STEP_SHR});
    }
    const ArithStepRec& last = rec[std::size_t(count - 1)];
    std::int64_t product = signExtend((last.A << n) | last.Q, 2 * n);
    std::ostringstream r;
    r << "A:Q = " << toBits(last.A, n) << " " << toBits(last.Q, n) << " = " << product
      << (product == m * q ? "   (correct: " : "   (expected ") << m << " x " << q << " = " << m * q << ")";
    t.result = r.str();
    t.ok = product == m * q;
    return t;
}

ArithTrace addShiftMultiply(unsigned mIn, unsigned qIn, int n) {
    ArithTrace t;
    t.title = "Add-and-Shift Multiplication";
    t.bits = n;
    t.columns = {"C", "A", "Q", "M"};
    std::uint64_t mask = (1ULL << n) - 1, M = mIn & mask, Q = qIn & mask;
    std::ostringstream sum;
    sum << "Unsigned " << M << " x " << Q << ". If Q0 = 1 then C,A = A + M; then logical shift right C A Q.";
    t.summary = sum.str();

    auto rec = records(std::size_t(n));
    int count = vas_add_shift(M, Q, n, rec.data());
    int cycle = 0;
    for (int i = 0; i < count; ++i) {
        const ArithStepRec& r = rec[std::size_t(i)];
        std::string act;
        switch (r.kind) {
            case STEP_INIT: act = "C = 0, A = 0, Q = multiplier, M = multiplicand"; break;
            case STEP_ADD:  act = "Q0 = 1  ->  C,A = A + M"; break;
            case STEP_NOP:  act = "Q0 = 0  ->  no addition"; break;
            default:        act = "Shift right  C A Q"; break;
        }
        if (r.kind == STEP_ADD || r.kind == STEP_NOP) ++cycle;
        t.steps.push_back({r.kind == STEP_INIT ? "init" : std::to_string(cycle),
                           {std::to_string(r.extra), toBits(r.A, n), toBits(r.Q, n), toBits(M, n)}, act,
                           r.kind == STEP_SHR});
    }
    const ArithStepRec& last = rec[std::size_t(count - 1)];
    std::uint64_t product = (last.A << n) | last.Q;
    std::ostringstream r;
    r << "A:Q = " << toBits(last.A, n) << " " << toBits(last.Q, n) << " = " << product << "   (" << M << " x " << Q
      << " = " << M * Q << ")";
    t.result = r.str();
    t.ok = product == M * Q;
    return t;
}

ArithTrace restoringDivide(unsigned dividendIn, unsigned divisorIn, int n) {
    ArithTrace t;
    t.title = "Restoring Division";
    t.bits = n;
    t.columns = {"A", "Q", "M"};
    std::uint64_t mask = (1ULL << n) - 1, Qin = dividendIn & mask, M = divisorIn & mask;
    std::ostringstream sum;
    sum << "Unsigned " << Qin << " / " << M << ". Shift A Q left, A = A - M; if A < 0 restore (A = A + M, Q0 = 0) else Q0 = 1.";
    t.summary = sum.str();
    if (M == 0) {
        t.ok = false;
        t.result = "Division by zero - the hardware raises a divide error (#DE)";
        return t;
    }

    auto rec = records(std::size_t(n));
    int count = vas_restoring_div(Qin, M, n, rec.data());
    int cycle = 0;
    for (int i = 0; i < count; ++i) {
        const ArithStepRec& r = rec[std::size_t(i)];
        std::string act;
        switch (r.kind) {
            case STEP_INIT:    act = "A = 0, Q = dividend, M = divisor"; break;
            case STEP_SHL:     act = "Shift left  A Q"; ++cycle; break;
            case STEP_SUB:     act = "A = A - M"; break;
            case STEP_RESTORE: act = "A < 0  ->  restore A = A + M,  Q0 = 0"; break;
            default:           act = "A >= 0  ->  Q0 = 1"; break;
        }
        t.steps.push_back({r.kind == STEP_INIT ? "init" : std::to_string(cycle),
                           {toBits(r.A, n + 1), toBits(r.Q, n), toBits(M, n)}, act, r.kind == STEP_SHL});
    }
    std::uint64_t Q = rec[std::size_t(count - 1)].Q, A = rec[std::size_t(count - 1)].A;
    std::ostringstream r;
    r << "Quotient Q = " << Q << ",  Remainder A = " << A << "   (" << Qin << " = " << M << " x " << Q << " + " << A << ")";
    t.result = r.str();
    t.ok = Q == Qin / M && A == Qin % M;
    return t;
}

NumberViews numberViews(std::int64_t v) {
    NumberViews n;
    n.dec = std::to_string(v);
    char b[40];
    std::snprintf(b, sizeof b, "%016llX", static_cast<unsigned long long>(v));
    n.hex = b;
    std::uint64_t u = std::uint64_t(v);
    int bits = 16;
    while (bits < 64 && (u >> bits) != 0 && v >= 0) bits += 16;
    if (v < 0) bits = 64;
    std::string s = toBits(u, bits);
    for (int i = int(s.size()) - 4; i > 0; i -= 4) s.insert(std::size_t(i), " ");
    n.bin = s;
    std::uint64_t mag = v < 0 ? std::uint64_t(-(v + 1)) + 1 : std::uint64_t(v);
    std::string digits = std::to_string(mag);
    if (digits.size() > 16) {
        n.bcdOk = false;
        n.bcd = "too many digits for 64-bit BCD";
    } else {
        std::uint64_t packed = vas_bin_to_bcd(mag);                 // arithmetic.asm
        std::string bcd;
        for (int i = int(digits.size()) - 1; i >= 0; --i) {
            if (!bcd.empty()) bcd += ' ';
            bcd += toBits((packed >> (4 * i)) & 0xF, 4);
        }
        n.bcd = (v < 0 ? "- " : "") + bcd;
        std::uint64_t back = vas_bcd_to_bin(packed);               // BCD -> binary
        std::snprintf(b, sizeof b, "packed BCD 0x%llX  ->  binary 0x%llX", static_cast<unsigned long long>(packed),
                      static_cast<unsigned long long>(back));
        n.bcdBack = b;
    }
    return n;
}

} // namespace vas::core
