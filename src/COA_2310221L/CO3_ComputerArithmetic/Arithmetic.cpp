#include "COA_2310221L/CO3_ComputerArithmetic/Arithmetic.h"

#include <cstdio>
#include <sstream>

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
    std::uint64_t mask = (1ULL << n) - 1;
    std::uint64_t M = std::uint64_t(mIn) & mask, A = 0, Q = std::uint64_t(qIn) & mask;
    int q1 = 0;
    std::int64_t m = signExtend(M, n), q = signExtend(Q, n);
    std::ostringstream sum;
    sum << "Signed " << m << " x " << q << " using " << n << "-bit registers. Look at Q0 Q-1: "
        << "10 -> A = A - M,  01 -> A = A + M,  then arithmetic shift right A Q Q-1." << widened;
    t.summary = sum.str();
    auto row = [&](const std::string& cyc, const std::string& act, bool shift) {
        t.steps.push_back({cyc, {toBits(A, n), toBits(Q, n), std::to_string(q1), toBits(M, n)}, act, shift});
    };
    row("init", "A = 0, Q = multiplier, Q-1 = 0, M = multiplicand", false);
    for (int c = 1; c <= n; ++c) {
        int q0 = int(Q & 1);
        std::string cyc = std::to_string(c);
        if (q0 == 1 && q1 == 0) { A = (A - M) & mask; row(cyc, "Q0 Q-1 = 10  ->  A = A - M", false); }
        else if (q0 == 0 && q1 == 1) { A = (A + M) & mask; row(cyc, "Q0 Q-1 = 01  ->  A = A + M", false); }
        else row(cyc, std::string("Q0 Q-1 = ") + char('0' + q0) + char('0' + q1) + "  ->  no operation", false);
        int signA = int((A >> (n - 1)) & 1);
        q1 = int(Q & 1);
        Q = ((Q >> 1) | ((A & 1) << (n - 1))) & mask;
        A = ((A >> 1) | (std::uint64_t(signA) << (n - 1))) & mask;
        row(cyc, "Arithmetic shift right  A Q Q-1", true);
    }
    std::int64_t product = signExtend((A << n) | Q, 2 * n);
    std::ostringstream r;
    r << "A:Q = " << toBits(A, n) << " " << toBits(Q, n) << " = " << product
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
    std::uint64_t mask = (1ULL << n) - 1;
    std::uint64_t M = mIn & mask, A = 0, Q = qIn & mask, C = 0;
    std::ostringstream sum;
    sum << "Unsigned " << M << " x " << Q << ". If Q0 = 1 then C,A = A + M; then logical shift right C A Q.";
    t.summary = sum.str();
    auto row = [&](const std::string& cyc, const std::string& act, bool shift) {
        t.steps.push_back({cyc, {std::to_string(C), toBits(A, n), toBits(Q, n), toBits(M, n)}, act, shift});
    };
    row("init", "C = 0, A = 0, Q = multiplier, M = multiplicand", false);
    for (int c = 1; c <= n; ++c) {
        std::string cyc = std::to_string(c);
        if (Q & 1) {
            std::uint64_t s = A + M;
            C = (s >> n) & 1;
            A = s & mask;
            row(cyc, "Q0 = 1  ->  C,A = A + M", false);
        } else {
            row(cyc, "Q0 = 0  ->  no addition", false);
        }
        Q = ((Q >> 1) | ((A & 1) << (n - 1))) & mask;
        A = ((A >> 1) | (C << (n - 1))) & mask;
        C = 0;
        row(cyc, "Shift right  C A Q", true);
    }
    std::uint64_t product = (A << n) | Q;
    std::ostringstream r;
    r << "A:Q = " << toBits(A, n) << " " << toBits(Q, n) << " = " << product << "   ("
      << (mIn & mask) << " x " << (qIn & mask) << " = " << (mIn & mask) * (qIn & mask) << ")";
    t.result = r.str();
    t.ok = product == std::uint64_t(mIn & mask) * (qIn & mask);
    return t;
}

ArithTrace restoringDivide(unsigned dividendIn, unsigned divisorIn, int n) {
    ArithTrace t;
    t.title = "Restoring Division";
    t.bits = n;
    t.columns = {"A", "Q", "M"};
    std::uint64_t mask = (1ULL << n) - 1, amask = (1ULL << (n + 1)) - 1;
    std::uint64_t Q = dividendIn & mask, M = divisorIn & mask, A = 0;
    std::ostringstream sum;
    sum << "Unsigned " << Q << " / " << M << ". Shift A Q left, A = A - M; if A < 0 restore (A = A + M, Q0 = 0) else Q0 = 1.";
    t.summary = sum.str();
    if (M == 0) {
        t.ok = false;
        t.result = "Division by zero - the hardware raises a divide error (#DE)";
        return t;
    }
    auto row = [&](const std::string& cyc, const std::string& act, bool shift) {
        t.steps.push_back({cyc, {toBits(A, n + 1), toBits(Q, n), toBits(M, n)}, act, shift});
    };
    row("init", "A = 0, Q = dividend, M = divisor", false);
    for (int c = 1; c <= n; ++c) {
        std::string cyc = std::to_string(c);
        A = ((A << 1) | ((Q >> (n - 1)) & 1)) & amask;
        Q = (Q << 1) & mask;
        row(cyc, "Shift left  A Q", true);
        A = (A - M) & amask;
        row(cyc, "A = A - M", false);
        if ((A >> n) & 1) {
            A = (A + M) & amask;
            row(cyc, "A < 0  ->  restore A = A + M,  Q0 = 0", false);
        } else {
            Q |= 1;
            row(cyc, "A >= 0  ->  Q0 = 1", false);
        }
    }
    std::ostringstream r;
    r << "Quotient Q = " << Q << ",  Remainder A = " << A << "   (" << (dividendIn & mask) << " = " << M << " x " << Q
      << " + " << A << ")";
    t.result = r.str();
    t.ok = Q == (dividendIn & mask) / M && A == (dividendIn & mask) % M;
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
        std::string bcd;
        std::uint64_t packed = 0;
        for (char d : digits) {
            if (!bcd.empty()) bcd += ' ';
            bcd += toBits(std::uint64_t(d - '0'), 4);
            packed = (packed << 4) | std::uint64_t(d - '0');
        }
        n.bcd = (v < 0 ? "- " : "") + bcd;
        // BCD -> hex: read each nibble as a decimal digit and rebuild the value
        std::uint64_t back = 0;
        for (char d : digits) back = back * 10 + std::uint64_t(d - '0');
        std::snprintf(b, sizeof b, "packed BCD 0x%llX  ->  binary 0x%llX", static_cast<unsigned long long>(packed),
                      static_cast<unsigned long long>(back));
        n.bcdBack = b;
    }
    return n;
}

} // namespace vas::core
