// Console tests for the simulator core (no graphics).
// Build: make test   ->   ./build/test_core
#include <cstdio>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "core/Arithmetic.h"
#include "core/Assembler.h"
#include "core/CPU.h"
#include "PSOOP_2310261L/CO3_ProblemSolving/Compiler.h"
#include "core/ControlUnit.h"
#include "PSOOP_2310261L/CO1_RealWorldOOP/English.h"
#include "core/Pipeline.h"
#include "PL_2310214L/CO4_SearchingSorting/Searching.h"
#include "PL_2310214L/CO4_SearchingSorting/Sorting.h"

using namespace vas;
using namespace vas::core;

static int failures = 0, passes = 0;
#define CHECK(cond, msg)                                                  \
    do {                                                                  \
        if (cond) ++passes;                                               \
        else { ++failures; std::printf("  FAIL: %s  (%s:%d)\n", msg, __FILE__, __LINE__); } \
    } while (0)

static std::map<std::string, std::int64_t> runSource(const std::string& src, bool show = false,
                                                     std::vector<ExecRecord>* trace = nullptr,
                                                     std::unique_ptr<Program>* keep = nullptr) {
    Compiler c;
    CompileResult r = c.compile(src, ds::HashMethod::Linear);
    if (!r.ok) {
        std::printf("  compile error: %s: %s (line %d)\n", r.errorKind.c_str(), r.error.c_str(), r.errorLine);
        ++failures;
        return {};
    }
    if (show) std::cout << r.asmText;
    auto prog = Assembler::assemble(r.asmText);
    CPU cpu;
    cpu.load(prog.get());
    while (!cpu.halted()) {
        ExecRecord rec = cpu.step();
        if (trace && rec.index >= 0) trace->push_back(rec);
    }
    std::map<std::string, std::int64_t> vars;
    for (const auto& d : prog->data) vars[d.name] = cpu.peek(d.addr);
    if (keep) *keep = std::move(prog);
    return vars;
}

int main() {
    std::puts("== compiler + CPU");
    {
        auto v = runSource("a = 5\nb = 7\nc = a + b * 2", true);
        CHECK(v["c"] == 19, "c = a + b * 2 == 19");
    }
    {
        auto v = runSource("x = (3 + 4) * (10 - 2) / 3 % 5");
        CHECK(v["x"] == ((3 + 4) * (10 - 2) / 3) % 5, "parenthesised expression");
    }
    {
        auto v = runSource("a = -20\nb = 6\nq = a / b\nr = a % b\nn = -a\nm = ~0");
        CHECK(v["q"] == -3 && v["r"] == -2, "signed division truncates toward zero");
        CHECK(v["n"] == 20 && v["m"] == -1, "unary minus / not");
    }
    {
        auto v = runSource("a = 12\nb = 10\nc = a & b\nd = a | b\ne = a ^ b\nf = a << 3\ng = 256 >> 4\ns = 3\nh = a << s");
        CHECK(v["c"] == 8 && v["d"] == 14 && v["e"] == 6, "bitwise ops");
        CHECK(v["f"] == 96 && v["g"] == 16 && v["h"] == 96, "shifts");
    }
    {
        auto v = runSource("a = 9\nb = 4\nc = 7\nif (a > b) { max = a } else { max = b }\nif (c > max) max = c", true);
        CHECK(v["max"] == 9, "greatest of three");
    }
    {
        std::vector<ExecRecord> trace;
        std::unique_ptr<Program> prog;
        auto v = runSource("n = 5\nf = 1\nwhile (n > 1) {\n  f = f * n\n  n = n - 1\n}", false, &trace, &prog);
        CHECK(v["f"] == 120, "factorial(5) == 120");
        PipelineConfig cfg;
        PipelineResult pr = PipelineSimulator::run(*prog, trace, cfg);
        std::printf("  pipeline fwd: instr=%d cycles=%d nonpipe=%d CPI=%.2f speedup=%.2f stalls=%d flushes=%d fwd=%d\n",
                    pr.instructions, pr.pipelinedCycles, pr.nonPipelinedCycles, pr.cpi(), pr.speedup(), pr.stalls,
                    pr.flushes, pr.forwardsUsed);
        cfg.forwarding = false;
        PipelineResult pn = PipelineSimulator::run(*prog, trace, cfg);
        std::printf("  pipeline no-fwd: cycles=%d CPI=%.2f stalls=%d\n", pn.pipelinedCycles, pn.cpi(), pn.stalls);
        CHECK(pn.pipelinedCycles >= pr.pipelinedCycles, "forwarding never slower");
        CHECK(pr.pipelinedCycles < pr.nonPipelinedCycles, "pipelining is faster");
        for (int i = 0; i < 8 && i < int(pr.rows.size()); ++i) {
            const auto& row = pr.rows[i];
            std::printf("    %-24s", row.text.c_str());
            for (int s = 0; s < STAGE_COUNT; ++s) std::printf(" %s:%d-%d", stageName(s), row.start[s], row.end[s]);
            std::printf("%s %s\n", row.flushed ? " FLUSH" : "", row.hazard.c_str());
        }
        // control unit sequences for every instruction kind in the program
        HardwiredControlUnit hw;
        MicroprogrammedControlUnit mp;
        int hwSteps = 0, mpSteps = 0;
        for (const auto& rec : trace) {
            hwSteps += int(hw.sequence(*prog->code[rec.index], rec).size());
            mpSteps += int(mp.sequence(*prog->code[rec.index], rec).size());
        }
        std::printf("  control unit: hardwired %d cycles, microprogrammed %d micro-steps for %zu instructions\n",
                    hwSteps, mpSteps, trace.size());
        CHECK(mpSteps > hwSteps, "microprogram needs more micro-steps");
    }
    {
        auto v = runSource("a = 6\nb = 7\np = 0\ni = 0\nwhile (i < b) { p = p + a  i = i + 1 }");
        CHECK(v["p"] == 42, "multiplication by successive addition");
    }
    {
        Compiler c;
        auto r = c.compile("a = 5 +", ds::HashMethod::Chaining);
        CHECK(!r.ok, "syntax error detected");
        std::printf("  error message: %s\n", r.error.c_str());
        r = c.compile("x = (1 + 2", ds::HashMethod::Chaining);
        CHECK(!r.ok, "missing paren detected");
        r = c.compile("rax = 3", ds::HashMethod::Chaining);
        CHECK(!r.ok, "register name rejected");
        r = c.compile("a = 1\nb = 0\nc = a / b", ds::HashMethod::Chaining);
        auto prog = Assembler::assemble(r.asmText);
        CPU cpu;
        cpu.load(prog.get());
        bool threw = false;
        try { while (!cpu.halted()) cpu.step(); } catch (const DivideByZeroError&) { threw = true; }
        CHECK(threw, "divide by zero raises DivideByZeroError");
    }

    std::puts("== English -> code");
    {
        struct Case { const char* english; const char* var; std::int64_t expect; };
        const Case cases[] = {
            {"add 12 and 3", "result", 15},
            {"set x to 5. add 3 to x", "x", 8},
            {"set a to 7\nset b to 4\nmultiply a by b", "a", 28},
            {"compute 2 plus 3 times 4 and store it in y", "y", 14},
            {"subtract 4 from 10 into d", "d", 6},
            {"divide 100 by 7 giving q", "q", 14},
            {"find the remainder of 100 divided by 7 as r", "r", 2},
            {"set x to 9. set y to 4. if x is greater than y then set m to x otherwise set m to y", "m", 9},
            {"find the factorial of 5", "factorial", 120},
            {"find the sum of numbers from 1 to 10", "sum", 55},
            {"find the largest of 4 and 19 and 7", "max", 19},
            {"set c to 0. repeat 4 times: increase c by 3", "c", 12},
            {"multiply 6 by 7 using repeated addition", "product", 42},
            {"set n to one hundred twenty three", "n", 123},
            {"raise 2 to the power of 10", "power", 1024},
            {"set p to 3. set q to 8. swap p and q", "p", 8},
            {"set i to 1. while i is less than 100 double i", "i", 128},
            {"x is 6. square x", "x", 36},
        };
        for (const auto& c : cases) {
            EnglishResult er = EnglishTranslator::translate(c.english);
            if (!er.ok) { std::printf("  FAIL translate \"%s\": %s\n", c.english, er.error.c_str()); ++failures; continue; }
            auto v = runSource(er.code);
            bool ok = v.count(c.var) && v[c.var] == c.expect;
            if (!ok) std::printf("  FAIL \"%s\" -> %s = %lld (expected %lld)\n%s", c.english, c.var,
                                 v.count(c.var) ? (long long)v[c.var] : -999LL, (long long)c.expect, er.code.c_str());
            CHECK(ok, c.english);
        }
        EnglishResult bad = EnglishTranslator::translate("fly to the moon");
        CHECK(!bad.ok, "unknown verb rejected");
        std::printf("  error message: %s\n", bad.error.c_str());
    }

    std::puts("== C++ -> code");
    {
        auto v = runSource("#include <iostream>\nusing namespace std;\nint main() {\n    int a = 5, b = 7;\n    int c;\n"
                           "    c = a + b * 2;\n    a += 10;\n    b--;\n    ++c;\n    for (int i = 0; i < 4; i++) {\n"
                           "        c += i;\n    }\n    cout << c << endl;\n    return 0;\n}\n");
        CHECK(v["a"] == 15 && v["b"] == 6, "compound assignment and decrement");
        CHECK(v["c"] == 20 + 6, "for loop with ++");
        v = runSource("long f = 1;\nfor (int n = 5; n > 1; n--) f *= n;");
        CHECK(v["f"] == 120, "for loop factorial without braces");
    }

    std::puts("== assembler (hand-written asm)");
    {
        const char* src =
            "section .data\n  n dq 10\n  sum dq 0\nsection .text\n_start:\n"
            "  mov rcx, [n]\n  xor rax, rax\nloop:\n  add rax, rcx\n  dec rcx\n  jnz loop\n"
            "  mov [sum], rax\n  push rax\n  call double\n  pop rbx\n  hlt\n"
            "double:\n  shl rax, 1\n  ret\n";
        auto prog = Assembler::assemble(src);
        CPU cpu;
        Profiler prof;
        cpu.addObserver(&prof);
        cpu.load(prog.get());
        while (!cpu.halted()) cpu.step();
        CHECK(cpu.peek(CPU::kDataBase + 8) == 55, "sum 1..10 == 55");
        CHECK(cpu.reg(RAX).u() == 110 && cpu.reg(RBX).u() == 55, "call/ret/push/pop");
        CHECK(cpu.reg(RSP).u() == CPU::kStackTop, "stack balanced");
        for (auto& [m, n] : prof.counts()) std::printf("  %s:%d", m.c_str(), n);
        std::puts("");
        bool threw = false;
        try { Assembler::assemble("mov eax, 5"); } catch (const SyntaxError& e) { threw = true; std::printf("  %s\n", e.what()); }
        CHECK(threw, "32-bit register rejected with a hint");
        threw = false;
        try { Assembler::assemble("foo rax, 1"); } catch (const InvalidOpcodeError&) { threw = true; }
        CHECK(threw, "unknown opcode rejected");
    }

    std::puts("== flags");
    {
        auto prog = Assembler::assemble("mov rax, 0x7FFFFFFFFFFFFFFF\nadd rax, 1\nhlt");
        CPU cpu;
        cpu.load(prog.get());
        cpu.step();
        cpu.step();
        CHECK(cpu.flags().OF && cpu.flags().SF && !cpu.flags().CF && !cpu.flags().ZF, "signed overflow flags");
        prog = Assembler::assemble("mov rax, 3\nsub rax, 5\nhlt");
        cpu.load(prog.get());
        cpu.step();
        cpu.step();
        CHECK(cpu.flags().CF && cpu.flags().SF && !cpu.flags().OF, "borrow flags");
    }

    std::puts("== arithmetic algorithms");
    for (int m : {7, -7, 13, -128, 0}) for (int q : {-3, 3, 5, -1, 127}) {
        auto t = boothMultiply(m, q, 8);
        if (!t.ok) std::printf("  booth %d x %d -> %s\n", m, q, t.result.c_str());
        CHECK(t.ok, "booth");
    }
    for (unsigned m : {13u, 255u, 0u, 7u}) for (unsigned q : {11u, 255u, 1u}) CHECK(addShiftMultiply(m, q, 8).ok, "add-shift");
    for (unsigned a : {100u, 255u, 7u, 0u}) for (unsigned b : {7u, 1u, 13u, 255u}) {
        auto t = restoringDivide(a, b, 8);
        if (!t.ok) std::printf("  div %u / %u -> %s\n", a, b, t.result.c_str());
        CHECK(t.ok, "restoring division");
    }
    std::printf("  %s\n", boothMultiply(7, -3).result.c_str());
    auto nv = numberViews(1234);
    std::printf("  1234 -> hex %s  bcd %s  %s\n", nv.hex.c_str(), nv.bcd.c_str(), nv.bcdBack.c_str());

    std::puts("== data structures");
    {
        std::vector<int> base = {42, 7, 19, 3, 88, 25, 61, 12, 5, 30};
        ds::BubbleSort b; ds::InsertionSort in; ds::QuickSort q;
        for (ds::Sorter* s : std::vector<ds::Sorter*>{&b, &in, &q}) {
            std::vector<int> a = base;
            std::vector<ds::SortStep> st;
            s->sort(a, st);
            bool sorted = true;
            for (std::size_t i = 1; i < a.size(); ++i) if (a[i - 1] > a[i]) sorted = false;
            CHECK(sorted, s->name().c_str());
        }
        std::vector<int> a = {3, 5, 7, 12, 19, 25, 30, 42, 61, 88};
        ds::LinearSearch ls; ds::BinarySearch bs; ds::FibonacciSearch fs;
        for (int t : {3, 88, 25, 4, 100, 42}) {
            std::vector<ds::SearchStep> s1, s2, s3;
            int r1 = ls.search(a, t, s1), r2 = bs.search(a, t, s2), r3 = fs.search(a, t, s3);
            if (!(r1 == r2 && r2 == r3)) std::printf("  search %d: %d %d %d\n", t, r1, r2, r3);
            CHECK(r1 == r2 && r2 == r3, "searches agree");
        }
        for (auto m : {ds::HashMethod::Linear, ds::HashMethod::Quadratic, ds::HashMethod::Chaining}) {
            std::unique_ptr<ds::HashTable> h(ds::makeHashTable(m, 5));
            const char* keys[] = {"a", "b", "c", "sum", "i", "n", "max", "count", "total", "x", "y"};
            int k = 0;
            for (const char* key : keys) h->insert(key, k++);
            bool ok = true;
            k = 0;
            for (const char* key : keys) { int v; if (!h->find(key, v) || v != k) ok = false; ++k; }
            CHECK(ok, h->name().c_str());
            std::printf("  %-18s cap=%d load=%.2f probes=%d collisions=%d\n", h->name().c_str(), h->capacity(),
                        h->loadFactor(), h->totalProbes(), h->collisions());
        }
    }

    std::printf("\n%d passed, %d failed\n", passes, failures);
    return failures ? 1 : 0;
}
