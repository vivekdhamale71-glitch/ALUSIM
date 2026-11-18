#include "core/Instruction.h"

#include <climits>

#include "engine/ds/Searching.h"
#include "engine/ds/Sorting.h"
#include "core/CPU.h"
#include "engine/lang/Errors.h"

namespace vas::core {

long Instruction::executedCount = 0;
long Instruction::classCount[kInstrClassCount] = {};

void Instruction::countExecution(InstrClass c) {
    ++executedCount;
    ++classCount[int(c)];
}

void Instruction::resetCounts() {
    executedCount = 0;
    for (long& n : classCount) n = 0;
}

// pseudo register used for hazard detection: the flags register
static constexpr int REG_FLAGS = REG_COUNT;

const char* regName(int r) {
    static const char* n[REG_COUNT] = {"rax", "rbx", "rcx", "rdx", "rsi", "rdi", "rsp", "rbp",
                                       "r8",  "r9",  "r10", "r11", "r12", "r13", "r14", "r15"};
    if (r == REG_FLAGS) return "flags";
    return (r >= 0 && r < REG_COUNT) ? n[r] : "?";
}

const char* instrClassName(InstrClass c) {
    switch (c) {
        case InstrClass::DataTransfer: return "Data Transfer";
        case InstrClass::Arithmetic: return "Arithmetic";
        case InstrClass::Logic: return "Logic";
        case InstrClass::ControlFlow: return "Control Flow";
        case InstrClass::Stack: return "Stack";
        case InstrClass::System: return "System";
    }
    return "?";
}

// ------------------------------------------------------------------ helpers
static void addUnique(std::vector<int>& v, int r) {
    if (r < 0) return;
    for (int x : v) if (x == r) return;
    v.push_back(r);
}
static void addMemBases(const std::vector<Operand>& ops, std::vector<int>& out) {
    for (const auto& o : ops) if (o.kind == OperandKind::Memory && o.reg >= 0) addUnique(out, o.reg);
}
static bool isReg(const Operand& o) { return o.kind == OperandKind::Register; }
static bool isMem(const Operand& o) { return o.kind == OperandKind::Memory; }
static bool isImm(const Operand& o) { return o.kind == OperandKind::Immediate; }

static void recordAlu(ExecRecord& rec, AluOp op, std::uint64_t a, std::uint64_t b, std::uint64_t out) {
    rec.aluUsed = true;
    rec.aluOp = op;
    rec.aluA = a;
    rec.aluB = b;
    rec.aluOut = out;
}

std::vector<int> Instruction::readRegs() const {
    std::vector<int> r;
    if (ops_.size() > 1 && isReg(ops_[1])) addUnique(r, ops_[1].reg);
    addMemBases(ops_, r);
    return r;
}
std::vector<int> Instruction::writeRegs() const {
    std::vector<int> r;
    if (!ops_.empty() && isReg(ops_[0])) addUnique(r, ops_[0].reg);
    return r;
}
bool Instruction::readsMemory() const { return signals().MemRead; }
bool Instruction::writesMemory() const { return signals().MemWrite; }

// ------------------------------------------------------------ data transfer
void DataTransferInstr::execute(CPU& cpu, ExecRecord& rec) const {
    const Operand& dst = ops_[0];
    const Operand& src = ops_[1];
    std::int64_t v = cpu.read(src, rec);
    if (isImm(src) || isReg(src)) recordAlu(rec, AluOp::Pass, 0, std::uint64_t(v), std::uint64_t(v));
    cpu.write(dst, v, rec);
}

ControlSignals DataTransferInstr::signals() const {
    ControlSignals s;
    const Operand& dst = ops_[0];
    const Operand& src = ops_[1];
    if (isReg(dst)) s.RegWrite = true;
    if (isMem(dst)) { s.MemWrite = true; s.ALUSrc = true; }
    if (isImm(src)) { s.ALUSrc = true; s.aluOp = AluOp::Pass; }
    if (isReg(src)) s.aluOp = AluOp::Pass;
    if (isMem(src)) { s.MemRead = true; s.MemToReg = true; s.ALUSrc = true; }
    return s;
}

// --------------------------------------------------------------- arithmetic
static AluOp arithOp(const std::string& m) {
    if (m == "add") return AluOp::Add;
    if (m == "sub" || m == "cmp") return AluOp::Sub;
    if (m == "imul") return AluOp::Mul;
    if (m == "idiv") return AluOp::Div;
    if (m == "inc") return AluOp::Inc;
    if (m == "dec") return AluOp::Dec;
    if (m == "neg") return AluOp::Neg;
    return AluOp::None;
}

void ArithmeticInstr::execute(CPU& cpu, ExecRecord& rec) const {
    if (mnem_ == "cqo") {
        std::int64_t rax = std::int64_t(cpu.getReg(RAX, rec));
        cpu.setReg(RDX, std::uint64_t(vas_cqo(rax)), rec);          // isa.asm
        return;
    }
    if (mnem_ == "idiv") {
        std::int64_t divisor = cpu.read(ops_[0], rec);
        if (divisor == 0) throw DivideByZeroError(line_);
        std::uint64_t rax = cpu.getReg(RAX, rec);
        std::uint64_t rdx = cpu.getReg(RDX, rec);
        std::int64_t qr[2];
        if (vas_idiv(rdx, rax, divisor, qr) != 0)                     // isa.asm
            throw SimulatorError("Divide Error (#DE)", "Quotient does not fit in RAX (did you forget CQO?)", line_);
        recordAlu(rec, AluOp::Div, rax, std::uint64_t(divisor), std::uint64_t(qr[0]));
        cpu.setReg(RAX, std::uint64_t(qr[0]), rec);
        cpu.setReg(RDX, std::uint64_t(qr[1]), rec);
        return;
    }
    AluOp op = arithOp(mnem_);
    const Operand& dst = ops_[0];
    Register64 a(std::uint64_t(cpu.read(dst, rec)));
    Register64 b(0);
    if (ops_.size() > 1) b = Register64(std::uint64_t(cpu.read(ops_[1], rec)));
    AluResult r = ALU::compute(op, a, b, cpu.flags());
    recordAlu(rec, op, a.u(), b.u(), r.value.u());
    if (mnem_ != "cmp") cpu.write(dst, r.value.s(), rec);
    cpu.setFlags(r.flags);
}

ControlSignals ArithmeticInstr::signals() const {
    ControlSignals s;
    if (mnem_ == "cqo") { s.RegWrite = true; s.aluOp = AluOp::Pass; return s; }
    s.aluOp = arithOp(mnem_);
    if (mnem_ == "idiv") {
        s.RegWrite = true;
        if (isMem(ops_[0])) { s.MemRead = true; s.ALUSrc = true; }
        return s;
    }
    s.FlagWrite = true;
    const Operand& dst = ops_[0];
    if (mnem_ != "cmp") {
        if (isReg(dst)) s.RegWrite = true;
        if (isMem(dst)) { s.MemRead = true; s.MemWrite = true; s.ALUSrc = true; }
    }
    if (ops_.size() > 1) {
        if (isImm(ops_[1])) s.ALUSrc = true;
        if (isMem(ops_[1])) { s.MemRead = true; s.ALUSrc = true; }
    }
    return s;
}

std::vector<int> ArithmeticInstr::readRegs() const {
    std::vector<int> r;
    if (mnem_ == "cqo") return {RAX};
    if (mnem_ == "idiv") { r = {RAX, RDX}; if (isReg(ops_[0])) addUnique(r, ops_[0].reg); addMemBases(ops_, r); return r; }
    if (isReg(ops_[0])) addUnique(r, ops_[0].reg);
    if (ops_.size() > 1 && isReg(ops_[1])) addUnique(r, ops_[1].reg);
    addMemBases(ops_, r);
    return r;
}
std::vector<int> ArithmeticInstr::writeRegs() const {
    if (mnem_ == "cqo") return {RDX};
    if (mnem_ == "idiv") return {RAX, RDX};
    std::vector<int> r;
    if (mnem_ != "cmp" && isReg(ops_[0])) addUnique(r, ops_[0].reg);
    addUnique(r, REG_FLAGS);
    return r;
}

// -------------------------------------------------------------------- logic
static AluOp logicOp(const std::string& m) {
    if (m == "and" || m == "test") return AluOp::And;
    if (m == "or") return AluOp::Or;
    if (m == "xor") return AluOp::Xor;
    if (m == "not") return AluOp::Not;
    if (m == "shl" || m == "sal") return AluOp::Shl;
    if (m == "shr") return AluOp::Shr;
    return AluOp::None;
}

void LogicInstr::execute(CPU& cpu, ExecRecord& rec) const {
    AluOp op = logicOp(mnem_);
    const Operand& dst = ops_[0];
    Register64 a(std::uint64_t(cpu.read(dst, rec)));
    Register64 b(0);
    if (ops_.size() > 1) {
        if (ops_[1].isCL) b = Register64(cpu.getReg(RCX, rec) & 0xFF);
        else b = Register64(std::uint64_t(cpu.read(ops_[1], rec)));
    }
    AluResult r = ALU::compute(op, a, b, cpu.flags());
    recordAlu(rec, op, a.u(), b.u(), r.value.u());
    if (mnem_ != "test") cpu.write(dst, r.value.s(), rec);
    cpu.setFlags(r.flags);
}

ControlSignals LogicInstr::signals() const {
    ControlSignals s;
    s.aluOp = logicOp(mnem_);
    s.FlagWrite = mnem_ != "not";
    const Operand& dst = ops_[0];
    if (mnem_ != "test") {
        if (isReg(dst)) s.RegWrite = true;
        if (isMem(dst)) { s.MemRead = true; s.MemWrite = true; s.ALUSrc = true; }
    }
    if (ops_.size() > 1) {
        if (isImm(ops_[1])) s.ALUSrc = true;
        if (isMem(ops_[1])) { s.MemRead = true; s.ALUSrc = true; }
    }
    return s;
}

std::vector<int> LogicInstr::readRegs() const {
    std::vector<int> r;
    if (isReg(ops_[0])) addUnique(r, ops_[0].reg);
    if (ops_.size() > 1) {
        if (ops_[1].isCL) addUnique(r, RCX);
        else if (isReg(ops_[1])) addUnique(r, ops_[1].reg);
    }
    addMemBases(ops_, r);
    return r;
}
std::vector<int> LogicInstr::writeRegs() const {
    std::vector<int> r;
    if (mnem_ != "test" && isReg(ops_[0])) addUnique(r, ops_[0].reg);
    if (mnem_ != "not") addUnique(r, REG_FLAGS);
    return r;
}

// ------------------------------------------------------------- control flow
bool ControlFlowInstr::conditionMet(const Flags& f) const {
    // condition code for isa.asm: 0 always 1 E 2 NE 3 G 4 GE 5 L 6 LE
    const std::string& m = mnem_;
    int cc = -1;
    if (m == "jmp" || m == "call" || m == "ret") cc = 0;
    else if (m == "je" || m == "jz") cc = 1;
    else if (m == "jne" || m == "jnz") cc = 2;
    else if (m == "jg") cc = 3;
    else if (m == "jge") cc = 4;
    else if (m == "jl") cc = 5;
    else if (m == "jle") cc = 6;
    return cc >= 0 && vas_condition(cc, f.rflags());
}

void ControlFlowInstr::execute(CPU& cpu, ExecRecord& rec) const {
    if (mnem_ == "ret") {
        std::int64_t addr = cpu.pop(rec);
        cpu.jump(CPU::indexOf(std::uint64_t(addr)), rec);
        return;
    }
    if (mnem_ == "call") {
        cpu.push(std::int64_t(CPU::addressOf(rec.index + 1)), rec);
        cpu.jump(ops_[0].target, rec);
        return;
    }
    if (conditionMet(cpu.flags())) cpu.jump(ops_[0].target, rec);
}

ControlSignals ControlFlowInstr::signals() const {
    ControlSignals s;
    if (mnem_ == "jmp") { s.Jump = true; s.PCWrite = true; return s; }
    if (mnem_ == "call") {
        s.Jump = true; s.PCWrite = true; s.StackOp = true; s.MemWrite = true; s.RegWrite = true;
        s.aluOp = AluOp::Sub; s.ALUSrc = true;
        return s;
    }
    if (mnem_ == "ret") {
        s.Jump = true; s.PCWrite = true; s.StackOp = true; s.MemRead = true; s.RegWrite = true;
        s.aluOp = AluOp::Add; s.ALUSrc = true;
        return s;
    }
    s.Branch = true;   // conditional jump: PC written only if the condition holds
    return s;
}

std::vector<int> ControlFlowInstr::readRegs() const {
    if (mnem_ == "call" || mnem_ == "ret") return {RSP};
    if (mnem_ == "jmp") return {};
    return {REG_FLAGS};
}
std::vector<int> ControlFlowInstr::writeRegs() const {
    if (mnem_ == "call" || mnem_ == "ret") return {RSP};
    return {};
}

// -------------------------------------------------------------------- stack
void StackInstr::execute(CPU& cpu, ExecRecord& rec) const {
    std::uint64_t rsp = cpu.reg(RSP).u();
    if (mnem_ == "push") {
        std::int64_t v = cpu.read(ops_[0], rec);
        recordAlu(rec, AluOp::Sub, rsp, 8, rsp - 8);
        cpu.push(v, rec);
    } else {
        recordAlu(rec, AluOp::Add, rsp, 8, rsp + 8);
        std::int64_t v = cpu.pop(rec);
        cpu.write(ops_[0], v, rec);
    }
}

ControlSignals StackInstr::signals() const {
    ControlSignals s;
    s.StackOp = true;
    s.RegWrite = true;
    s.ALUSrc = true;
    if (mnem_ == "push") { s.MemWrite = true; s.aluOp = AluOp::Sub; }
    else { s.MemRead = true; s.MemToReg = true; s.aluOp = AluOp::Add; }
    return s;
}

std::vector<int> StackInstr::readRegs() const {
    std::vector<int> r = {RSP};
    if (mnem_ == "push" && isReg(ops_[0])) addUnique(r, ops_[0].reg);
    addMemBases(ops_, r);
    return r;
}
std::vector<int> StackInstr::writeRegs() const {
    std::vector<int> r = {RSP};
    if (mnem_ == "pop" && isReg(ops_[0])) addUnique(r, ops_[0].reg);
    return r;
}

// ------------------------------------------------------------------- system
void SystemInstr::execute(CPU& cpu, ExecRecord&) const {
    if (mnem_ == "hlt") cpu.halt();
}

// ------------------------------------------------------------------ creators
template <typename T>
static std::unique_ptr<Instruction> makeInstr(const std::string& m, std::vector<Operand> o, int line,
                                              const std::string& text) {
    return std::make_unique<T>(m, std::move(o), line, text);
}

static InstrCreator creatorFor(InstrClass k) {
    switch (k) {
        case InstrClass::DataTransfer: return &makeInstr<DataTransferInstr>;
        case InstrClass::Arithmetic: return &makeInstr<ArithmeticInstr>;
        case InstrClass::Logic: return &makeInstr<LogicInstr>;
        case InstrClass::ControlFlow: return &makeInstr<ControlFlowInstr>;
        case InstrClass::Stack: return &makeInstr<StackInstr>;
        case InstrClass::System: return &makeInstr<SystemInstr>;
    }
    return nullptr;
}

// ------------------------------------------------------------- opcode table
OpcodeTable::OpcodeTable() {
    using C = InstrClass;
    table_ = {
        {"mov", C::DataTransfer, 2, 2, "89 /r", "Copy source into destination"},
        {"add", C::Arithmetic, 2, 2, "01 /r", "dst = dst + src, sets CF ZF SF OF"},
        {"sub", C::Arithmetic, 2, 2, "29 /r", "dst = dst - src, sets CF ZF SF OF"},
        {"imul", C::Arithmetic, 2, 2, "0F AF /r", "Signed multiply dst = dst * src"},
        {"idiv", C::Arithmetic, 1, 1, "F7 /7", "Signed divide RDX:RAX by src -> RAX quotient, RDX remainder"},
        {"inc", C::Arithmetic, 1, 1, "FF /0", "dst = dst + 1 (CF unchanged)"},
        {"dec", C::Arithmetic, 1, 1, "FF /1", "dst = dst - 1 (CF unchanged)"},
        {"neg", C::Arithmetic, 1, 1, "F7 /3", "Two's complement negate"},
        {"cmp", C::Arithmetic, 2, 2, "39 /r", "Compute dst - src, set flags only"},
        {"cqo", C::Arithmetic, 0, 0, "48 99", "Sign-extend RAX into RDX:RAX"},
        {"and", C::Logic, 2, 2, "21 /r", "Bitwise AND"},
        {"or", C::Logic, 2, 2, "09 /r", "Bitwise OR"},
        {"xor", C::Logic, 2, 2, "31 /r", "Bitwise XOR"},
        {"not", C::Logic, 1, 1, "F7 /2", "Bitwise NOT (flags unchanged)"},
        {"shl", C::Logic, 2, 2, "D3 /4", "Shift left, last bit out -> CF"},
        {"sal", C::Logic, 2, 2, "D3 /4", "Arithmetic shift left (same as SHL)"},
        {"shr", C::Logic, 2, 2, "D3 /5", "Logical shift right, last bit out -> CF"},
        {"test", C::Logic, 2, 2, "85 /r", "Compute dst AND src, set flags only"},
        {"jmp", C::ControlFlow, 1, 1, "E9 cd", "Unconditional jump"},
        {"je", C::ControlFlow, 1, 1, "0F 84", "Jump if equal (ZF=1)"},
        {"jz", C::ControlFlow, 1, 1, "0F 84", "Jump if zero (ZF=1)"},
        {"jne", C::ControlFlow, 1, 1, "0F 85", "Jump if not equal (ZF=0)"},
        {"jnz", C::ControlFlow, 1, 1, "0F 85", "Jump if not zero (ZF=0)"},
        {"jg", C::ControlFlow, 1, 1, "0F 8F", "Jump if greater (ZF=0 and SF=OF)"},
        {"jge", C::ControlFlow, 1, 1, "0F 8D", "Jump if greater or equal (SF=OF)"},
        {"jl", C::ControlFlow, 1, 1, "0F 8C", "Jump if less (SF!=OF)"},
        {"jle", C::ControlFlow, 1, 1, "0F 8E", "Jump if less or equal (ZF=1 or SF!=OF)"},
        {"call", C::ControlFlow, 1, 1, "E8 cd", "Push return address, jump to procedure"},
        {"ret", C::ControlFlow, 0, 0, "C3", "Pop return address into RIP"},
        {"push", C::Stack, 1, 1, "50+r", "RSP -= 8, [RSP] = src"},
        {"pop", C::Stack, 1, 1, "58+r", "dst = [RSP], RSP += 8"},
        {"nop", C::System, 0, 0, "90", "No operation"},
        {"hlt", C::System, 0, 0, "F4", "Halt the processor"},
    };
    for (auto& e : table_) e.create = creatorFor(e.klass);
    // sorted once, so every lookup can use binary search            [PL CO4]
    // (this sorted array is the ONLY mnemonic -> instruction mapping)
    ds::quickSort(table_, [](const OpcodeInfo& a, const OpcodeInfo& b) { return a.mnemonic < b.mnemonic; });
}

const OpcodeTable& OpcodeTable::instance() {
    static OpcodeTable t;
    return t;
}

const OpcodeInfo* OpcodeTable::lookup(const std::string& mnem, std::vector<int>* probes) const {
    int i = ds::binarySearch(table_, mnem,
                             [](const OpcodeInfo& e, const std::string& k) { return e.mnemonic.compare(k); }, probes);
    return i < 0 ? nullptr : &table_[i];
}

// ------------------------------------------------------------------ factory
const InstructionFactory& InstructionFactory::instance() {
    static InstructionFactory f;
    return f;
}

std::unique_ptr<Instruction> InstructionFactory::create(const OpcodeInfo& info, std::vector<Operand> ops, int line,
                                                        const std::string& text) const {
    return info.create(info.mnemonic, std::move(ops), line, text);   // the subclass for this class of opcode
}

std::unique_ptr<Instruction> InstructionFactory::create(const std::string& mnem, std::vector<Operand> ops, int line,
                                                        const std::string& text) const {
    const OpcodeInfo* info = OpcodeTable::instance().lookup(mnem);  // binary search
    if (!info) throw InvalidOpcodeError("Unknown instruction '" + mnem + "'", line);
    return create(*info, std::move(ops), line, text);
}

} // namespace vas::core
