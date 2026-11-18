// Runs the three CO4 programs on the simulator core and writes their traces (signals, micro-steps, pipeline) as JSON.
// Built and run by docs/make_coa_co4_pdf.py.
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "core/Assembler.h"
#include "core/CPU.h"
#include "core/ControlUnit.h"
#include "core/Pipeline.h"
#include "engine/lang/Compiler.h"

using namespace vas::core;

static std::string esc(const std::string& s) {
    std::string r;
    for (char c : s) {
        if (c == '"' || c == '\\') r += '\\';
        if (c == '\n') { r += "\\n"; continue; }
        r += c;
    }
    return r;
}

static std::string sigJson(const ControlSignals& s) {
    std::ostringstream o;
    o << "{";
    for (int i = 0; i < ControlSignals::kCount; ++i) o << "\"" << ControlSignals::name(i) << "\":" << (s.get(i) ? 1 : 0) << ",";
    o << "\"ALUOp\":\"" << aluOpName(s.aluOp) << "\"}";
    return o.str();
}

static std::string stepsJson(const std::vector<MicroStep>& st) {
    std::ostringstream o;
    o << "[";
    for (std::size_t i = 0; i < st.size(); ++i) {
        o << (i ? "," : "") << "{\"phase\":\"" << phaseName(st[i].phase) << "\",\"op\":\"" << esc(st[i].microOp)
          << "\",\"addr\":" << st[i].microAddr << ",\"next\":" << st[i].nextAddr << ",\"sig\":" << sigJson(st[i].sig) << "}";
    }
    o << "]";
    return o.str();
}

static std::string pipeJson(const PipelineResult& r) {
    std::ostringstream o;
    o << "{\"cycles\":" << r.pipelinedCycles << ",\"plain\":" << r.nonPipelinedCycles << ",\"n\":" << r.instructions
      << ",\"stalls\":" << r.stalls << ",\"flushes\":" << r.flushes << ",\"fwd\":" << r.forwardsUsed << ",\"rows\":[";
    for (std::size_t i = 0; i < r.rows.size(); ++i) {
        const PipeRow& p = r.rows[i];
        o << (i ? "," : "") << "{\"text\":\"" << esc(p.text) << "\",\"flushed\":" << (p.flushed ? 1 : 0) << ",\"hazard\":\""
          << esc(p.hazard) << "\",\"start\":[";
        for (int k = 0; k < STAGE_COUNT; ++k) o << (k ? "," : "") << p.start[k];
        o << "],\"end\":[";
        for (int k = 0; k < STAGE_COUNT; ++k) o << (k ? "," : "") << p.end[k];
        o << "]}";
    }
    o << "]}";
    return o.str();
}

int main() {
    struct Job { const char* name; const char* src; bool isAsm; };
    std::vector<Job> jobs = {
        {"Arithmetic expression", "a = 5\nb = 7\nc = a + b * 2", false},
        {"Loop: sum of 1 to n", "n = 4\nsum = 0\nwhile (n > 0) {\n  sum = sum + n\n  n = n - 1\n}", false},
        {"Procedure call: CALL / RET with the stack",
         "section .data\n    n    dq 5\n    out  dq 0\n\nsection .text\n_start:\n    mov rcx, [n]\n    push rcx\n    call double\n"
         "    pop rbx\n    mov [out], rax\n    hlt\n\ndouble:\n    mov rax, rcx\n    shl rax, 1\n    ret\n",
         true}};
    HardwiredControlUnit hw;
    MicroprogrammedControlUnit mp;
    std::ostringstream out;
    out << "{\"decoder\":[";
    const auto& tbl = HardwiredControlUnit::decoderTable();
    for (std::size_t i = 0; i < tbl.size(); ++i)
        out << (i ? "," : "") << "{\"form\":\"" << esc(tbl[i].form) << "\",\"sig\":" << sigJson(tbl[i].sig) << "}";
    out << "],\"store\":[";
    const auto& cs = mp.controlStore();
    for (std::size_t i = 0; i < cs.size(); ++i)
        out << (i ? "," : "") << "{\"addr\":" << cs[i].addr << ",\"label\":\"" << esc(cs[i].label) << "\",\"op\":\""
            << esc(cs[i].microOp) << "\",\"phase\":\"" << phaseName(cs[i].phase) << "\",\"next\":\"" << esc(cs[i].next)
            << "\",\"sig\":" << sigJson(cs[i].sig) << "}";
    out << "],\"jobs\":[";
    for (std::size_t j = 0; j < jobs.size(); ++j) {
        std::string asmText = jobs[j].src;
        if (!jobs[j].isAsm) {
            Compiler c;
            CompileResult r = c.compile(jobs[j].src, vas::ds::HashMethod::Linear);
            if (!r.ok) { std::fprintf(stderr, "compile error %s\n", r.error.c_str()); return 1; }
            asmText = r.asmText;
        }
        auto prog = Assembler::assemble(asmText);
        CPU cpu;
        cpu.load(prog.get());
        std::vector<ExecRecord> trace;
        while (!cpu.halted()) {
            ExecRecord rec = cpu.step();
            if (rec.index >= 0) trace.push_back(rec);
        }
        out << (j ? "," : "") << "{\"name\":\"" << jobs[j].name << "\",\"source\":\"" << esc(jobs[j].src) << "\",\"asm\":\""
            << esc(asmText) << "\",\"data\":{";
        bool first = true;
        for (const auto& d : prog->data) { out << (first ? "" : ",") << "\"" << d.name << "\":" << cpu.peek(d.addr); first = false; }
        out << "},\"trace\":[";
        for (std::size_t i = 0; i < trace.size(); ++i) {
            const ExecRecord& r = trace[i];
            Instruction* ins = prog->code[std::size_t(r.index)].get();
            out << (i ? "," : "") << "{\"i\":" << i << ",\"text\":\"" << esc(r.text) << "\",\"alu\":\"" << aluOpName(r.aluOp)
                << "\",\"aluA\":" << (long long)r.aluA << ",\"aluB\":" << (long long)r.aluB << ",\"aluOut\":" << (long long)r.aluOut
                << ",\"flags\":[" << r.flagsAfter.CF << "," << r.flagsAfter.ZF << "," << r.flagsAfter.SF << "," << r.flagsAfter.OF << "],\"taken\":" << (r.branchTaken ? 1 : 0) << ",\"sig\":" << sigJson(r.sig) << ",\"hw\":" << stepsJson(hw.sequence(*ins, r))
                << ",\"mp\":" << stepsJson(mp.sequence(*ins, r)) << "}";
        }
        out << "],\"pipe\":{";
        PipelineConfig a; a.forwarding = false;
        PipelineConfig b; b.forwarding = true;
        out << "\"noFwd\":" << pipeJson(PipelineSimulator::run(*prog, trace, a)) << ",\"fwd\":" << pipeJson(PipelineSimulator::run(*prog, trace, b)) << "}}";
    }
    out << "]}";
    std::ofstream(std::getenv("CO4_JSON") ? std::getenv("CO4_JSON") : "co4.json") << out.str();
    std::puts("ok");
}
