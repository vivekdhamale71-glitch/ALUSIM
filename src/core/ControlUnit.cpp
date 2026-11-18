#include "core/ControlUnit.h"

#include <cctype>
#include <cstdio>
#include <initializer_list>

#include "core/CPU.h"

namespace vas::core {

const char* phaseName(Phase p) {
    switch (p) {
        case Phase::Fetch: return "FETCH";
        case Phase::Decode: return "DECODE";
        case Phase::Execute: return "EXECUTE";
        case Phase::Memory: return "MEMORY";
        case Phase::WriteBack: return "WRITE-BACK";
    }
    return "?";
}

const char* unitName(int u) {
    static const char* n[UNIT_COUNT] = {"PC", "Instr Memory", "IR", "Control Unit", "Register File", "Imm / Sign-Ext",
                                        "ALUSrc MUX", "ALU", "Flags", "Data Memory", "WB MUX"};
    return (u >= 0 && u < UNIT_COUNT) ? n[u] : "?";
}

std::string fmtValue(std::int64_t v) {
    char b[40];
    if (v > -1000000 && v < 1000000) std::snprintf(b, sizeof b, "%lld", static_cast<long long>(v));
    else std::snprintf(b, sizeof b, "0x%llX", static_cast<unsigned long long>(v));
    return b;
}
static std::string hx(std::uint64_t v) {
    char b[40];
    std::snprintf(b, sizeof b, "0x%llX", static_cast<unsigned long long>(v));
    return b;
}
static unsigned U(std::initializer_list<int> l) {
    unsigned m = 0;
    for (int u : l) m |= 1u << u;
    return m;
}

// ----------------------------------------------------------- instruction form
namespace {

enum Form {
    F_ALU_RR, F_ALU_RI, F_ALU_RM, F_CMP, F_UNARY, F_MOV_RR, F_MOV_RI, F_LOAD, F_STORE,
    F_JCC, F_JMP, F_PUSH, F_POP, F_CALL, F_RET, F_IDIV, F_CQO, F_HLT, F_NOP
};

bool isMem(const Operand& o) { return o.kind == OperandKind::Memory; }
bool isImm(const Operand& o) { return o.kind == OperandKind::Immediate; }

Form formOf(const Instruction& ins) {
    const std::string& m = ins.mnemonic();
    const auto& ops = ins.operands();
    if (m == "mov") {
        if (isMem(ops[0])) return F_STORE;
        if (isMem(ops[1])) return F_LOAD;
        if (isImm(ops[1])) return F_MOV_RI;
        return F_MOV_RR;
    }
    if (m == "cmp" || m == "test") return F_CMP;
    if (m == "inc" || m == "dec" || m == "neg" || m == "not") return F_UNARY;
    if (m == "idiv") return F_IDIV;
    if (m == "cqo") return F_CQO;
    if (m == "jmp") return F_JMP;
    if (m == "call") return F_CALL;
    if (m == "ret") return F_RET;
    if (m == "push") return F_PUSH;
    if (m == "pop") return F_POP;
    if (m == "hlt") return F_HLT;
    if (m == "nop") return F_NOP;
    if (ins.klass() == InstrClass::ControlFlow) return F_JCC;
    if (ops.size() > 1 && (isMem(ops[1]) || isMem(ops[0]))) return F_ALU_RM;
    if (ops.size() > 1 && isImm(ops[1])) return F_ALU_RI;
    return F_ALU_RR;
}

std::string upper(std::string s) {
    for (auto& c : s) c = char(std::toupper(static_cast<unsigned char>(c)));
    return s;
}

std::string destName(const Instruction& ins) {
    const auto& o = ins.operands();
    if (o.empty()) return "";
    if (o[0].kind == OperandKind::Register) return upper(regName(o[0].reg));
    return "[" + o[0].symbol + "]";
}
std::string srcName(const Instruction& ins) {
    const auto& o = ins.operands();
    if (o.size() < 2) return "";
    if (o[1].kind == OperandKind::Register) return o[1].isCL ? "CL" : upper(regName(o[1].reg));
    if (o[1].kind == OperandKind::Immediate) return "IMM(" + std::to_string(o[1].imm) + ")";
    return "MDR";
}

std::uint64_t memAddr(const ExecRecord& r) { return r.mem.empty() ? 0 : r.mem[0].addr; }
std::int64_t memVal(const ExecRecord& r) { return r.mem.empty() ? 0 : r.mem[0].value; }

std::string flagText(const Flags& f) {
    return std::string("ZF=") + (f.ZF ? "1" : "0") + " SF=" + (f.SF ? "1" : "0") + " OF=" + (f.OF ? "1" : "0") +
           " CF=" + (f.CF ? "1" : "0");
}

std::string writtenText(const ExecRecord& r) {
    std::string s;
    for (int reg : r.regsWritten) {
        if (!s.empty()) s += " ;  ";
        s += upper(regName(reg)) + " <- " + fmtValue(std::int64_t(r.regAfter[reg]));
    }
    return s;
}

MicroStep mk(Phase ph, std::string op, std::string detail, std::vector<int> buses, unsigned units,
             std::string value) {
    MicroStep m;
    m.phase = ph;
    m.microOp = std::move(op);
    m.detail = std::move(detail);
    m.buses = std::move(buses);
    m.units = units;
    m.value = std::move(value);
    return m;
}

// common EXECUTE step for an ALU operation using register or immediate source
MicroStep aluStep(const Instruction& ins, const ExecRecord& r, bool srcFromMemory) {
    const auto& ops = ins.operands();
    bool imm = ops.size() > 1 && isImm(ops[1]);
    std::vector<int> b = {B_REGA_ALU, B_MUX_ALU};
    unsigned u = U({U_REGS, U_MUX, U_ALU});
    if (srcFromMemory) { b.push_back(B_DMEM_MUX); u |= U({U_DMEM}); }
    else if (imm) { b.push_back(B_IR_IMM); b.push_back(B_IMM_MUX); u |= U({U_IMM}); }
    else if (ops.size() > 1) b.push_back(B_REGB_MUX);
    ControlSignals s = ins.signals();
    ControlSignals e;
    e.aluOp = r.aluOp;
    e.ALUSrc = imm || srcFromMemory;
    e.FlagWrite = s.FlagWrite;
    if (e.FlagWrite) { b.push_back(B_ALU_FLAGS); u |= U({U_FLAGS}); }
    std::string sym = aluOpSymbol(r.aluOp);
    std::string op = "ALUout <- " + destName(ins) + " " + sym + " " + (ops.size() > 1 ? srcName(ins) : "");
    if (ops.size() == 1) op = "ALUout <- " + std::string(aluOpName(r.aluOp)) + " " + destName(ins);
    std::string detail = std::string("ALU ") + aluOpName(r.aluOp) + ":  " + fmtValue(std::int64_t(r.aluA)) + " " +
                         sym + " " + (ops.size() > 1 ? fmtValue(std::int64_t(r.aluB)) : "") + " = " +
                         fmtValue(std::int64_t(r.aluOut));
    if (e.FlagWrite) detail += "     flags: " + flagText(r.flagsAfter);
    MicroStep m = mk(Phase::Execute, op, detail, b, u, fmtValue(std::int64_t(r.aluOut)));
    m.sig = e;
    return m;
}

MicroStep addressStep(const Instruction& ins, const ExecRecord& r, Phase ph = Phase::Execute) {
    const Operand* mop = nullptr;
    for (const auto& o : ins.operands()) if (isMem(o)) mop = &o;
    std::vector<int> b = {B_IR_IMM, B_IMM_MUX, B_MUX_ALU, B_ALU_DMEM};
    unsigned u = U({U_IR, U_IMM, U_MUX, U_ALU, U_DMEM});
    if (mop && mop->reg >= 0) { b.push_back(B_REGA_ALU); u |= U({U_REGS}); }
    std::string what = mop ? mop->text : "operand";
    MicroStep m = mk(ph, "MAR <- address of " + what, "Effective address of " + what + " = " + hx(memAddr(r)), b, u,
                     hx(memAddr(r)));
    m.sig.ALUSrc = true;
    m.sig.aluOp = AluOp::Add;
    return m;
}

} // namespace

// ------------------------------------------------------------ hardwired CU
// The decoder ROM itself is in engine/asm/CO4_ControlUnitPipeline/control_unit.asm:
// vas_decode(form) returns the 11 control signals (bits 0..10) and the ALU
// operation (bits 16..23). Here each word is unpacked for the views.
const std::vector<DecoderRow>& HardwiredControlUnit::decoderTable() {
    static std::vector<DecoderRow> rows = [] {
        static const char* forms[] = {"ALU  reg, reg", "ALU  reg, imm", "ALU  reg, [mem]", "CMP / TEST",
                                      "INC DEC NEG NOT", "MOV  reg, reg", "MOV  reg, imm", "MOV  reg, [mem]",
                                      "MOV  [mem], src", "Jcc  (conditional)", "JMP", "PUSH", "POP", "CALL",
                                      "RET", "IDIV", "CQO", "HLT", "NOP"};
        std::vector<DecoderRow> out;
        for (int form = 0; form < vas_decode_rows(); ++form) {
            std::uint32_t w = vas_decode(form);
            ControlSignals s;
            bool* f[ControlSignals::kCount] = {&s.PCWrite, &s.IRWrite, &s.MemRead, &s.MemWrite, &s.RegWrite, &s.ALUSrc,
                                               &s.MemToReg, &s.Branch, &s.Jump, &s.FlagWrite, &s.StackOp};
            for (int i = 0; i < ControlSignals::kCount; ++i) *f[i] = (w >> i) & 1;
            s.aluOp = AluOp((w >> 16) & 0xFF);
            out.push_back({forms[form], s});
        }
        return out;
    }();
    return rows;
}

int HardwiredControlUnit::decoderRow(const Instruction& ins) { return int(formOf(ins)); }

std::vector<MicroStep> HardwiredControlUnit::sequence(const Instruction& ins, const ExecRecord& r) const {
    std::vector<MicroStep> s;
    Form f = formOf(ins);
    const auto& ops = ins.operands();
    std::string mn = upper(ins.mnemonic());

    MicroStep fe = mk(Phase::Fetch, "IR <- IMEM[RIP] ;  RIP <- RIP + 4",
                      "Fetch the instruction at " + hx(r.pcAddr) + " and advance RIP",
                      {B_PC_IMEM, B_IMEM_IR, B_PC_INC, B_INC_PC}, U({U_PC, U_IMEM, U_IR}), r.text);
    fe.sig.PCWrite = fe.sig.IRWrite = true;
    s.push_back(fe);

    std::vector<int> db = {B_IR_CU};
    unsigned du = U({U_IR, U_CU});
    if (!ins.readRegs().empty()) { db.push_back(B_IR_REGS); du |= U({U_REGS}); }
    bool hasImm = false;
    for (const auto& o : ops) if (isImm(o) || isMem(o) || o.kind == OperandKind::Label) hasImm = true;
    if (hasImm) { db.push_back(B_IR_IMM); du |= U({U_IMM}); }
    s.push_back(mk(Phase::Decode, "Decode " + mn + "  ->  row \"" + decoderTable()[f].form + "\"",
                   std::string(instrClassName(ins.klass())) + " instruction - the decoder turns the opcode into the "
                                                              "control-signal vector and the register file is read",
                   db, du, mn));

    auto wb = [&](bool fromMem) {
        if (r.regsWritten.empty()) return;
        std::vector<int> b = {B_WB_REGS, fromMem ? B_DMEM_WB : B_ALU_WB};
        MicroStep m = mk(Phase::WriteBack, writtenText(r), "Write the result back into the register file", b,
                         U({U_WB, U_REGS}), fmtValue(std::int64_t(r.regAfter[r.regsWritten.back()])));
        m.sig.RegWrite = true;
        m.sig.MemToReg = fromMem;
        s.push_back(m);
    };

    switch (f) {
        case F_ALU_RR: case F_ALU_RI: case F_CMP: case F_UNARY: {
            bool srcMem = ops.size() > 1 && isMem(ops[1]);
            bool dstMem = isMem(ops[0]);
            if (srcMem || dstMem) {
                s.push_back(addressStep(ins, r));
                MicroStep rd = mk(Phase::Memory, "MDR <- MEM[" + hx(memAddr(r)) + "]",
                                  "Read " + fmtValue(memVal(r)) + " from data memory", {B_ALU_DMEM, B_DMEM_MUX},
                                  U({U_DMEM}), fmtValue(memVal(r)));
                rd.sig.MemRead = true;
                s.push_back(rd);
            }
            s.push_back(aluStep(ins, r, srcMem));
            if (dstMem && f != F_CMP) {
                MicroStep w = mk(Phase::Memory, "MEM[" + hx(memAddr(r)) + "] <- ALUout", "Store the result",
                                 {B_ALU_DMEM}, U({U_ALU, U_DMEM}), fmtValue(std::int64_t(r.aluOut)));
                w.sig.MemWrite = true;
                s.push_back(w);
            }
            if (!dstMem && f != F_CMP) wb(false);
            break;
        }
        case F_ALU_RM: {
            bool dstMem = isMem(ops[0]);
            s.push_back(addressStep(ins, r));
            MicroStep rd = mk(Phase::Memory, "MDR <- MEM[" + hx(memAddr(r)) + "]",
                              "Read " + fmtValue(memVal(r)) + " from data memory into MDR", {B_ALU_DMEM, B_DMEM_MUX},
                              U({U_DMEM, U_MUX}), fmtValue(memVal(r)));
            rd.sig.MemRead = true;
            s.push_back(rd);
            s.push_back(aluStep(ins, r, !dstMem));
            if (dstMem) {
                MicroStep w = mk(Phase::Memory, "MEM[" + hx(memAddr(r)) + "] <- ALUout", "Store the result",
                                 {B_ALU_DMEM}, U({U_ALU, U_DMEM}), fmtValue(std::int64_t(r.aluOut)));
                w.sig.MemWrite = true;
                s.push_back(w);
            } else {
                wb(false);
            }
            break;
        }
        case F_MOV_RR: case F_MOV_RI: {
            bool imm = f == F_MOV_RI;
            std::vector<int> b = {B_MUX_ALU};
            unsigned u = U({U_MUX, U_ALU});
            if (imm) { b.push_back(B_IR_IMM); b.push_back(B_IMM_MUX); u |= U({U_IMM}); }
            else { b.push_back(B_REGB_MUX); u |= U({U_REGS}); }
            MicroStep m = mk(Phase::Execute, "ALUout <- PASS " + srcName(ins),
                             "The value " + fmtValue(std::int64_t(r.aluOut)) + " passes through the ALU unchanged", b, u,
                             fmtValue(std::int64_t(r.aluOut)));
            m.sig.ALUSrc = imm;
            m.sig.aluOp = AluOp::Pass;
            s.push_back(m);
            wb(false);
            break;
        }
        case F_LOAD: {
            s.push_back(addressStep(ins, r));
            MicroStep rd = mk(Phase::Memory, "MDR <- MEM[" + hx(memAddr(r)) + "]",
                              "Data memory returns " + fmtValue(memVal(r)), {B_ALU_DMEM, B_DMEM_WB}, U({U_DMEM}),
                              fmtValue(memVal(r)));
            rd.sig.MemRead = true;
            s.push_back(rd);
            wb(true);
            break;
        }
        case F_STORE: {
            s.push_back(addressStep(ins, r));
            bool imm = isImm(ops[1]);
            MicroStep w = mk(Phase::Memory, "MEM[" + hx(memAddr(r)) + "] <- " + srcName(ins),
                             "Write " + fmtValue(memVal(r)) + " into data memory",
                             imm ? std::vector<int>{B_ALU_DMEM, B_IR_IMM} : std::vector<int>{B_ALU_DMEM, B_REGB_DMEM},
                             U({U_DMEM, imm ? U_IMM : U_REGS}), fmtValue(memVal(r)));
            w.sig.MemWrite = true;
            s.push_back(w);
            break;
        }
        case F_JCC: {
            bool taken = r.branchTaken;
            std::vector<int> b = {B_FLAGS_CU};
            unsigned u = U({U_FLAGS, U_CU});
            if (taken) { b.push_back(B_IR_PC); u |= U({U_PC, U_IR}); }
            MicroStep m = mk(Phase::Execute,
                             taken ? "condition TRUE  ->  RIP <- " + ops[0].symbol : "condition FALSE  ->  fall through",
                             mn + " tests the flags (" + flagText(r.flagsBefore) + ")" +
                                 (taken ? " - branch taken" : " - branch not taken"),
                             b, u, taken ? "TAKEN" : "NOT TAKEN");
            m.sig.Branch = true;
            m.sig.PCWrite = taken;
            s.push_back(m);
            break;
        }
        case F_JMP: {
            MicroStep m = mk(Phase::Execute, "RIP <- " + ops[0].symbol,
                             "Unconditional jump to " + hx(CPU::addressOf(ops[0].target)), {B_IR_PC},
                             U({U_IR, U_PC}), ops[0].symbol);
            m.sig.Jump = m.sig.PCWrite = true;
            s.push_back(m);
            break;
        }
        case F_PUSH: case F_CALL: {
            MicroStep a = mk(Phase::Execute, "RSP <- RSP - 8",
                             "ALU computes the new stack top " + hx(r.aluOut ? r.aluOut : memAddr(r)),
                             {B_REGA_ALU, B_IMM_MUX, B_MUX_ALU, B_ALU_DMEM}, U({U_REGS, U_IMM, U_MUX, U_ALU}),
                             hx(memAddr(r)));
            a.sig.ALUSrc = true;
            a.sig.aluOp = AluOp::Sub;
            a.sig.StackOp = true;
            if (f == F_CALL) {
                a.buses.push_back(B_IR_PC);
                a.units |= U({U_PC, U_IR});
                a.microOp += " ;  RIP <- " + ops[0].symbol;
                a.sig.Jump = a.sig.PCWrite = true;
            }
            s.push_back(a);
            MicroStep w = mk(Phase::Memory,
                             f == F_CALL ? "MEM[RSP] <- return address" : "MEM[RSP] <- " + destName(ins),
                             "Push " + fmtValue(memVal(r)) + " onto the stack at " + hx(memAddr(r)),
                             {B_ALU_DMEM, B_REGB_DMEM}, U({U_DMEM, U_REGS}), fmtValue(memVal(r)));
            w.sig.MemWrite = w.sig.StackOp = true;
            s.push_back(w);
            wb(false);
            break;
        }
        case F_POP: case F_RET: {
            MicroStep a = mk(Phase::Execute, "MAR <- RSP", "The stack top " + hx(memAddr(r)) + " addresses memory",
                             {B_REGA_ALU, B_MUX_ALU, B_ALU_DMEM}, U({U_REGS, U_ALU}), hx(memAddr(r)));
            a.sig.StackOp = true;
            a.sig.aluOp = AluOp::Add;
            s.push_back(a);
            MicroStep rd = mk(Phase::Memory, "MDR <- MEM[RSP]", "Pop " + fmtValue(memVal(r)) + " off the stack",
                              {B_ALU_DMEM, B_DMEM_WB}, U({U_DMEM}), fmtValue(memVal(r)));
            rd.sig.MemRead = rd.sig.StackOp = true;
            s.push_back(rd);
            if (f == F_RET) {
                MicroStep j = mk(Phase::WriteBack, "RIP <- MDR ;  RSP <- RSP + 8",
                                 "Return to " + hx(std::uint64_t(memVal(r))), {B_DMEM_WB, B_IR_PC, B_WB_REGS},
                                 U({U_PC, U_REGS, U_WB}), hx(std::uint64_t(memVal(r))));
                j.sig.Jump = j.sig.PCWrite = j.sig.RegWrite = true;
                s.push_back(j);
            } else {
                wb(true);
            }
            break;
        }
        case F_IDIV: {
            std::vector<int> b = {B_REGA_ALU, B_MUX_ALU};
            if (isMem(ops[0])) {
                s.push_back(addressStep(ins, r));
                MicroStep rd = mk(Phase::Memory, "MDR <- MEM[" + hx(memAddr(r)) + "]", "Read the divisor",
                                  {B_ALU_DMEM, B_DMEM_MUX}, U({U_DMEM}), fmtValue(memVal(r)));
                rd.sig.MemRead = true;
                s.push_back(rd);
                b.push_back(B_DMEM_MUX);
            } else {
                b.push_back(B_REGB_MUX);
            }
            std::int64_t q = std::int64_t(r.regAfter[RAX]), rem = std::int64_t(r.regAfter[RDX]);
            MicroStep m = mk(Phase::Execute, "ALU: RDX:RAX / " + (isMem(ops[0]) ? std::string("MDR") : destName(ins)),
                             "Signed divide " + fmtValue(std::int64_t(r.aluA)) + " / " +
                                 fmtValue(std::int64_t(r.aluB)) + "  ->  quotient " + fmtValue(q) + ", remainder " +
                                 fmtValue(rem),
                             b, U({U_REGS, U_MUX, U_ALU}), fmtValue(q));
            m.sig.aluOp = AluOp::Div;
            s.push_back(m);
            wb(false);
            break;
        }
        case F_CQO: {
            MicroStep m = mk(Phase::Execute, "RDX <- sign(RAX)",
                             "CQO copies the sign bit of RAX into every bit of RDX (RDX:RAX = 128-bit dividend)",
                             {B_REGA_ALU, B_ALU_WB}, U({U_REGS, U_ALU}), fmtValue(std::int64_t(r.regAfter[RDX])));
            m.sig.aluOp = AluOp::Pass;
            s.push_back(m);
            wb(false);
            break;
        }
        case F_HLT:
            s.push_back(mk(Phase::Execute, "HALT", "The processor stops fetching instructions", {}, U({U_CU}), "HLT"));
            break;
        case F_NOP:
            s.push_back(mk(Phase::Execute, "(no operation)", "Nothing happens for one cycle", {}, U({U_CU}), "NOP"));
            break;
    }
    return s;
}

// ------------------------------------------------------ microprogrammed CU
void MicroprogrammedControlUnit::add(const std::string& label, const std::string& op, Phase ph, ControlSignals sig,
                                     std::vector<int> buses, unsigned units, const std::string& next) {
    store_.push_back({int(store_.size()), label, op, ph, sig, std::move(buses), units, next});
}

MicroprogrammedControlUnit::MicroprogrammedControlUnit() {
    auto S = [](std::initializer_list<int> on, AluOp op = AluOp::None) {
        ControlSignals s;
        bool* f[ControlSignals::kCount] = {&s.PCWrite, &s.IRWrite, &s.MemRead, &s.MemWrite, &s.RegWrite, &s.ALUSrc,
                                           &s.MemToReg, &s.Branch, &s.Jump, &s.FlagWrite, &s.StackOp};
        for (int i : on) *f[i] = true;
        s.aluOp = op;
        return s;
    };
    using P = Phase;
    // 0 PCWrite 1 IRWrite 2 MemRead 3 MemWrite 4 RegWrite 5 ALUSrc 6 MemToReg 7 Branch 8 Jump 9 FlagWrite 10 StackOp
    add("FETCH0", "MAR <- RIP", P::Fetch, S({}), {B_PC_IMEM}, U({U_PC, U_IMEM}));                         // 0
    add("FETCH1", "MDR <- IMEM[MAR] ;  RIP <- RIP + 4", P::Fetch, S({0}), {B_PC_INC, B_INC_PC}, U({U_PC, U_IMEM}));  // 1
    add("FETCH2", "IR <- MDR ;  uPC <- DISPATCH(opcode)", P::Decode, S({1}), {B_IMEM_IR, B_IR_CU},
        U({U_IMEM, U_IR, U_CU}), "DISPATCH");                                                                 // 2
    dispatchRom_.put("alu", int(store_.size()));
    add("ALU0", "A <- R[dst] ;  B <- R[src] / IMM", P::Decode, S({}), {B_IR_REGS}, U({U_REGS}));               // 3
    add("ALU1", "ALUout <- A op B ;  FLAGS <- status", P::Execute, S({9}), {B_REGA_ALU, B_MUX_ALU, B_ALU_FLAGS},
        U({U_ALU, U_MUX, U_FLAGS}));                                                                         // 4
    add("ALU2", "R[dst] <- ALUout", P::WriteBack, S({4}), {B_ALU_WB, B_WB_REGS}, U({U_WB, U_REGS}), "FETCH");  // 5
    dispatchRom_.put("alu.m", int(store_.size()));
    add("ALUM0", "MAR <- effective address", P::Execute, S({5}, AluOp::Add), {B_IR_IMM, B_IMM_MUX, B_MUX_ALU, B_ALU_DMEM},
        U({U_IMM, U_MUX, U_ALU, U_DMEM}));                                                                   // 6
    add("ALUM1", "MDR <- MEM[MAR]", P::Memory, S({2}), {B_ALU_DMEM, B_DMEM_MUX}, U({U_DMEM}));               // 7
    add("ALUM2", "ALUout <- R[dst] op MDR ;  FLAGS", P::Execute, S({5, 9}), {B_REGA_ALU, B_DMEM_MUX, B_MUX_ALU, B_ALU_FLAGS},
        U({U_REGS, U_MUX, U_ALU, U_FLAGS}));                                                                 // 8
    add("ALUM3", "R[dst] <- ALUout", P::WriteBack, S({4}), {B_ALU_WB, B_WB_REGS}, U({U_WB, U_REGS}), "FETCH"); // 9
    dispatchRom_.put("un", int(store_.size()));
    add("UN0", "A <- R[dst]", P::Decode, S({}), {B_IR_REGS}, U({U_REGS}));                                  // 10
    add("UN1", "ALUout <- op A ;  FLAGS", P::Execute, S({9}), {B_REGA_ALU, B_ALU_FLAGS}, U({U_ALU, U_FLAGS}));  // 11
    add("UN2", "R[dst] <- ALUout", P::WriteBack, S({4}), {B_ALU_WB, B_WB_REGS}, U({U_WB, U_REGS}), "FETCH");  // 12
    dispatchRom_.put("mov", int(store_.size()));
    add("MOV0", "ALUout <- PASS R[src] / IMM", P::Execute, S({}, AluOp::Pass), {B_MUX_ALU}, U({U_MUX, U_ALU}));  // 13
    add("MOV1", "R[dst] <- ALUout", P::WriteBack, S({4}), {B_ALU_WB, B_WB_REGS}, U({U_WB, U_REGS}), "FETCH");   // 14
    dispatchRom_.put("load", int(store_.size()));
    add("LD0", "MAR <- effective address", P::Execute, S({5}, AluOp::Add), {B_IR_IMM, B_IMM_MUX, B_MUX_ALU, B_ALU_DMEM},
        U({U_IMM, U_MUX, U_ALU, U_DMEM}));                                                                   // 15
    add("LD1", "MDR <- MEM[MAR]", P::Memory, S({2}), {B_ALU_DMEM, B_DMEM_WB}, U({U_DMEM}));                 // 16
    add("LD2", "R[dst] <- MDR", P::WriteBack, S({4, 6}), {B_DMEM_WB, B_WB_REGS}, U({U_WB, U_REGS}), "FETCH");   // 17
    dispatchRom_.put("store", int(store_.size()));
    add("ST0", "MAR <- effective address ;  MDR <- src", P::Execute, S({5}, AluOp::Add),
        {B_IR_IMM, B_IMM_MUX, B_MUX_ALU, B_ALU_DMEM, B_REGB_DMEM}, U({U_IMM, U_MUX, U_ALU, U_REGS}));         // 18
    add("ST1", "MEM[MAR] <- MDR", P::Memory, S({3}), {B_ALU_DMEM}, U({U_DMEM}), "FETCH");                   // 19
    dispatchRom_.put("br", int(store_.size()));
    add("BR0", "if cond(FLAGS) then RIP <- target", P::Execute, S({7}), {B_FLAGS_CU}, U({U_FLAGS, U_CU}), "FETCH");  // 20
    dispatchRom_.put("jmp", int(store_.size()));
    add("JMP0", "RIP <- target", P::Execute, S({0, 8}), {B_IR_PC}, U({U_IR, U_PC}), "FETCH");               // 21
    dispatchRom_.put("push", int(store_.size()));
    add("PUSH0", "RSP <- RSP - 8", P::Execute, S({4, 5, 10}, AluOp::Sub), {B_REGA_ALU, B_IMM_MUX, B_MUX_ALU, B_ALU_WB, B_WB_REGS},
        U({U_REGS, U_IMM, U_MUX, U_ALU, U_WB}));                                                             // 22
    add("PUSH1", "MAR <- RSP ;  MDR <- R[src]", P::Memory, S({10}), {B_ALU_DMEM, B_REGB_DMEM}, U({U_REGS, U_DMEM}));  // 23
    add("PUSH2", "MEM[MAR] <- MDR", P::Memory, S({3, 10}), {B_ALU_DMEM}, U({U_DMEM}), "FETCH");             // 24
    dispatchRom_.put("pop", int(store_.size()));
    add("POP0", "MAR <- RSP", P::Execute, S({10}), {B_REGA_ALU, B_MUX_ALU, B_ALU_DMEM}, U({U_REGS, U_ALU}));  // 25
    add("POP1", "MDR <- MEM[MAR]", P::Memory, S({2, 10}), {B_ALU_DMEM, B_DMEM_WB}, U({U_DMEM}));            // 26
    add("POP2", "R[dst] <- MDR", P::WriteBack, S({4, 6}), {B_DMEM_WB, B_WB_REGS}, U({U_WB, U_REGS}));       // 27
    add("POP3", "RSP <- RSP + 8", P::WriteBack, S({4, 5, 10}, AluOp::Add), {B_REGA_ALU, B_ALU_WB, B_WB_REGS},
        U({U_REGS, U_ALU, U_WB}), "FETCH");                                                                   // 28
    dispatchRom_.put("call", int(store_.size()));
    add("CALL0", "RSP <- RSP - 8", P::Execute, S({4, 5, 10}, AluOp::Sub), {B_REGA_ALU, B_IMM_MUX, B_MUX_ALU, B_ALU_WB, B_WB_REGS},
        U({U_REGS, U_IMM, U_MUX, U_ALU, U_WB}));                                                             // 29
    add("CALL1", "MEM[RSP] <- RIP (return address)", P::Memory, S({3, 10}), {B_ALU_DMEM, B_PC_INC}, U({U_PC, U_DMEM}));  // 30
    add("CALL2", "RIP <- target", P::Execute, S({0, 8}), {B_IR_PC}, U({U_IR, U_PC}), "FETCH");              // 31
    dispatchRom_.put("ret", int(store_.size()));
    add("RET0", "MDR <- MEM[RSP]", P::Memory, S({2, 10}), {B_REGA_ALU, B_ALU_DMEM, B_DMEM_WB}, U({U_REGS, U_DMEM}));  // 32
    add("RET1", "RSP <- RSP + 8", P::WriteBack, S({4, 5, 10}, AluOp::Add), {B_REGA_ALU, B_ALU_WB, B_WB_REGS},
        U({U_REGS, U_ALU, U_WB}));                                                                           // 33
    add("RET2", "RIP <- MDR", P::Execute, S({0, 8}), {B_DMEM_WB, B_IR_PC}, U({U_PC}), "FETCH");             // 34
    dispatchRom_.put("idiv", int(store_.size()));
    add("DIV0", "A <- RDX:RAX ;  B <- divisor", P::Decode, S({}), {B_IR_REGS, B_REGA_ALU, B_REGB_MUX}, U({U_REGS, U_MUX}));  // 35
    add("DIV1", "ALUout <- A / B ;  REM <- A mod B", P::Execute, S({}, AluOp::Div), {B_MUX_ALU}, U({U_ALU}));   // 36
    add("DIV2", "RAX <- quotient ;  RDX <- remainder", P::WriteBack, S({4}), {B_ALU_WB, B_WB_REGS}, U({U_WB, U_REGS}),
        "FETCH");                                                                                            // 37
    dispatchRom_.put("cqo", int(store_.size()));
    add("CQO0", "RDX <- sign-extend(RAX)", P::Execute, S({4}, AluOp::Pass), {B_REGA_ALU, B_ALU_WB, B_WB_REGS},
        U({U_REGS, U_ALU, U_WB}), "FETCH");                                                                   // 38
    dispatchRom_.put("hlt", int(store_.size()));
    add("HLT0", "halt", P::Execute, S({}), {}, U({U_CU}), "HALT");                                           // 39
    dispatchRom_.put("nop", int(store_.size()));
    add("NOP0", "(no operation)", P::Execute, S({}), {}, U({U_CU}), "FETCH");                               // 40
}

std::string MicroprogrammedControlUnit::routineKey(const Instruction& ins) {
    switch (formOf(ins)) {
        case F_ALU_RR: case F_ALU_RI: return "alu";
        case F_CMP: return (ins.operands().size() > 1 && isMem(ins.operands()[1])) ? "alu.m" : "alu";
        case F_ALU_RM: return "alu.m";
        case F_UNARY: return "un";
        case F_MOV_RR: case F_MOV_RI: return "mov";
        case F_LOAD: return "load";
        case F_STORE: return "store";
        case F_JCC: return "br";
        case F_JMP: return "jmp";
        case F_PUSH: return "push";
        case F_POP: return "pop";
        case F_CALL: return "call";
        case F_RET: return "ret";
        case F_IDIV: return "idiv";
        case F_CQO: return "cqo";
        case F_HLT: return "hlt";
        case F_NOP: return "nop";
    }
    return "nop";
}

int MicroprogrammedControlUnit::dispatch(const Instruction& ins) const {
    int a = 0;
    dispatchRom_.get(routineKey(ins), a);
    return a;
}

std::vector<MicroStep> MicroprogrammedControlUnit::sequence(const Instruction& ins, const ExecRecord& r) const {
    std::vector<MicroStep> out;
    Form f = formOf(ins);
    const auto& ops = ins.operands();
    bool imm = ops.size() > 1 && isImm(ops[1]);

    auto emit = [&](int addr, std::string value, std::string detail, int next) {
        const MicroInstr& mi = store_[addr];
        MicroStep m;
        m.phase = mi.phase;
        m.microOp = mi.microOp;
        m.sig = mi.sig;
        m.buses = mi.buses;
        m.units = mi.units;
        m.value = std::move(value);
        m.detail = std::move(detail);
        m.microAddr = addr;
        m.nextAddr = next;
        out.push_back(m);
        return &out.back();
    };

    int start = dispatch(ins);
    emit(0, hx(r.pcAddr), "Copy RIP into the memory address register", 1);
    emit(1, r.text, "Read the instruction word; RIP now points to the next instruction", 2);
    emit(2, upper(ins.mnemonic()),
         "Load IR; the dispatch ROM maps '" + routineKey(ins) + "' to micro-address " + std::to_string(start), start);

    switch (f) {
        case F_ALU_RR: case F_ALU_RI: case F_CMP: case F_UNARY: case F_ALU_RM: {
            bool memForm = routineKey(ins) == "alu.m";
            bool unary = f == F_UNARY;
            bool noWrite = f == F_CMP;
            if (memForm) {
                emit(6, hx(memAddr(r)), "Effective address " + hx(memAddr(r)), 7);
                emit(7, fmtValue(memVal(r)), "Data memory returns " + fmtValue(memVal(r)), 8);
                MicroStep* m = emit(8, fmtValue(std::int64_t(r.aluOut)),
                                    std::string("ALU ") + aluOpName(r.aluOp) + ": " + fmtValue(std::int64_t(r.aluA)) +
                                        " " + aluOpSymbol(r.aluOp) + " " + fmtValue(std::int64_t(r.aluB)) + " = " +
                                        fmtValue(std::int64_t(r.aluOut)),
                                    noWrite ? 0 : 9);
                m->sig.aluOp = r.aluOp;
                if (!noWrite) emit(9, fmtValue(std::int64_t(r.aluOut)), writtenText(r), 0);
            } else if (unary) {
                emit(10, fmtValue(std::int64_t(r.aluA)), "Read " + destName(ins), 11);
                MicroStep* m = emit(11, fmtValue(std::int64_t(r.aluOut)),
                                    std::string("ALU ") + aluOpName(r.aluOp) + " " + fmtValue(std::int64_t(r.aluA)) +
                                        " = " + fmtValue(std::int64_t(r.aluOut)),
                                    12);
                m->sig.aluOp = r.aluOp;
                m->sig.FlagWrite = ins.signals().FlagWrite;
                emit(12, fmtValue(std::int64_t(r.aluOut)), writtenText(r), 0);
            } else {
                MicroStep* a = emit(3, fmtValue(std::int64_t(r.aluA)) + ", " + fmtValue(std::int64_t(r.aluB)),
                                    "Read the operands from the register file" + std::string(imm ? " and the immediate" : ""), 4);
                if (imm) { a->buses.push_back(B_IR_IMM); a->units |= U({U_IMM}); }
                MicroStep* m = emit(4, fmtValue(std::int64_t(r.aluOut)),
                                    std::string("ALU ") + aluOpName(r.aluOp) + ": " + fmtValue(std::int64_t(r.aluA)) +
                                        " " + aluOpSymbol(r.aluOp) + " " + fmtValue(std::int64_t(r.aluB)) + " = " +
                                        fmtValue(std::int64_t(r.aluOut)) + "   flags: " + flagText(r.flagsAfter),
                                    noWrite ? 0 : 5);
                m->sig.aluOp = r.aluOp;
                m->sig.ALUSrc = imm;
                m->buses.push_back(imm ? B_IMM_MUX : B_REGB_MUX);
                if (!noWrite) emit(5, fmtValue(std::int64_t(r.aluOut)), writtenText(r), 0);
            }
            break;
        }
        case F_MOV_RR: case F_MOV_RI: {
            MicroStep* m = emit(13, fmtValue(std::int64_t(r.aluOut)), "The value passes through the ALU", 14);
            m->sig.ALUSrc = imm;
            if (imm) { m->buses.push_back(B_IR_IMM); m->buses.push_back(B_IMM_MUX); m->units |= U({U_IMM}); }
            else { m->buses.push_back(B_REGB_MUX); m->units |= U({U_REGS}); }
            emit(14, fmtValue(std::int64_t(r.aluOut)), writtenText(r), 0);
            break;
        }
        case F_LOAD:
            emit(15, hx(memAddr(r)), "Effective address " + hx(memAddr(r)), 16);
            emit(16, fmtValue(memVal(r)), "Data memory returns " + fmtValue(memVal(r)), 17);
            emit(17, fmtValue(memVal(r)), writtenText(r), 0);
            break;
        case F_STORE:
            emit(18, hx(memAddr(r)), "Address " + hx(memAddr(r)) + ", data " + fmtValue(memVal(r)), 19);
            emit(19, fmtValue(memVal(r)), "Write " + fmtValue(memVal(r)) + " into memory", 0);
            break;
        case F_JCC: {
            MicroStep* m = emit(20, r.branchTaken ? "TAKEN" : "NOT TAKEN",
                                upper(ins.mnemonic()) + " tests " + flagText(r.flagsBefore) +
                                    (r.branchTaken ? " - jump to " + ops[0].symbol : " - fall through"),
                                0);
            if (r.branchTaken) { m->buses.push_back(B_IR_PC); m->units |= U({U_PC}); m->sig.PCWrite = true; }
            break;
        }
        case F_JMP: emit(21, ops[0].symbol, "Jump to " + ops[0].symbol, 0); break;
        case F_PUSH:
            emit(22, hx(memAddr(r)), "New stack top " + hx(memAddr(r)), 23);
            emit(23, fmtValue(memVal(r)), "Prepare to store " + fmtValue(memVal(r)), 24);
            emit(24, fmtValue(memVal(r)), "Stack memory written", 0);
            break;
        case F_POP:
            emit(25, hx(memAddr(r)), "Address the stack top", 26);
            emit(26, fmtValue(memVal(r)), "Read " + fmtValue(memVal(r)), 27);
            emit(27, fmtValue(memVal(r)), "Write it to " + destName(ins), 28);
            emit(28, hx(r.regAfter[RSP]), "Release the stack slot", 0);
            break;
        case F_CALL:
            emit(29, hx(memAddr(r)), "Make room for the return address", 30);
            emit(30, fmtValue(memVal(r)), "Save the return address " + hx(std::uint64_t(memVal(r))), 31);
            emit(31, ops[0].symbol, "Enter the procedure", 0);
            break;
        case F_RET:
            emit(32, hx(std::uint64_t(memVal(r))), "Read the return address", 33);
            emit(33, hx(r.regAfter[RSP]), "Release the stack slot", 34);
            emit(34, hx(std::uint64_t(memVal(r))), "Return to the caller", 0);
            break;
        case F_IDIV:
            emit(35, fmtValue(std::int64_t(r.aluA)) + " / " + fmtValue(std::int64_t(r.aluB)), "Read dividend and divisor", 36);
            emit(36, fmtValue(std::int64_t(r.regAfter[RAX])),
                 "Quotient " + fmtValue(std::int64_t(r.regAfter[RAX])) + ", remainder " +
                     fmtValue(std::int64_t(r.regAfter[RDX])),
                 37);
            emit(37, fmtValue(std::int64_t(r.regAfter[RAX])), writtenText(r), 0);
            break;
        case F_CQO: emit(38, fmtValue(std::int64_t(r.regAfter[RDX])), "Sign-extend RAX into RDX", 0); break;
        case F_HLT: emit(39, "HLT", "The processor stops", -1); break;
        case F_NOP: emit(40, "NOP", "One idle cycle", 0); break;
    }
    return out;
}

} // namespace vas::core
