#include "COA_2310221L/CO1_Architecture/CPU.h"

#include <cstdio>
#include <sstream>

namespace vas::core {

static std::string hex64(std::uint64_t v) {
    char b[32];
    std::snprintf(b, sizeof b, "0x%llX", static_cast<unsigned long long>(v));
    return b;
}

CPU::CPU() { reset(); }

void CPU::load(const Program* prog) {
    prog_ = prog;
    reset();
}

void CPU::reset() {
    for (auto& r : regs_) r = Register64(0);
    regs_[RSP] = Register64(kStackTop);
    flags_ = Flags{};
    pc_ = nextPc_ = 0;
    halted_ = false;
    steps_ = 0;
    mem_.clear();
    if (prog_)
        for (const auto& d : prog_->data) mem_.put(d.addr, d.init);
    for (auto* o : observers_) o->onReset();
}

ExecRecord CPU::step() {
    if (!prog_) throw SimulatorError("No Program", "Compile a statement or assemble a program first");
    ExecRecord rec;
    if (halted_ || pc_ < 0 || pc_ >= int(prog_->code.size())) {
        halted_ = true;
        rec.halted = true;
        return rec;
    }
    if (steps_ >= kMaxSteps) throw RuntimeLimitError(steps_);

    const Instruction& ins = *prog_->code[pc_];
    rec.index = pc_;
    rec.pcAddr = addressOf(pc_);
    rec.text = ins.text();
    rec.mnemonic = ins.mnemonic();
    rec.klass = ins.klass();
    rec.sig = ins.signals();
    rec.flagsBefore = flags_;
    for (int i = 0; i < REG_COUNT; ++i) rec.regBefore[i] = regs_[i].u();

    nextPc_ = pc_ + 1;
    ins.execute(*this, rec);

    pc_ = nextPc_;
    ++steps_;
    ++Instruction::executedCount;
    rec.stepNo = steps_;
    rec.nextIndex = pc_;
    rec.flagsAfter = flags_;
    for (int i = 0; i < REG_COUNT; ++i) rec.regAfter[i] = regs_[i].u();
    if (pc_ >= int(prog_->code.size())) halted_ = true;
    rec.halted = halted_;
    for (auto* o : observers_) o->onStep(rec);
    return rec;
}

int CPU::currentLine() const {
    if (!prog_ || pc_ < 0 || pc_ >= int(prog_->instrLine.size())) return -1;
    return prog_->instrLine[pc_];
}

std::int64_t CPU::peek(std::uint64_t addr) const {
    const std::int64_t* v = mem_.find(addr);
    return v ? *v : 0;
}

Snapshot CPU::snapshot() const {
    Snapshot s;
    for (int i = 0; i < REG_COUNT; ++i) s.regs[i] = regs_[i].u();
    s.flags = flags_;
    s.pc = pc_;
    s.halted = halted_;
    s.steps = steps_;
    s.mem = mem_;
    return s;
}

void CPU::restore(const Snapshot& s) {
    for (int i = 0; i < REG_COUNT; ++i) regs_[i] = Register64(s.regs[i]);
    flags_ = s.flags;
    pc_ = nextPc_ = s.pc;
    halted_ = s.halted;
    steps_ = s.steps;
    mem_ = s.mem;
}

std::uint64_t CPU::getReg(int r, ExecRecord& rec) {
    bool seen = false;
    for (int x : rec.regsRead) if (x == r) seen = true;
    if (!seen) rec.regsRead.push_back(r);
    return regs_[r].u();
}

void CPU::setReg(int r, std::uint64_t v, ExecRecord& rec) {
    bool seen = false;
    for (int x : rec.regsWritten) if (x == r) seen = true;
    if (!seen) rec.regsWritten.push_back(r);
    regs_[r] = Register64(v);
}

std::uint64_t CPU::effectiveAddress(const Operand& op) const {
    std::uint64_t base = op.reg >= 0 ? regs_[op.reg].u() : 0;
    return base + op.symbolAddr + std::uint64_t(op.imm);
}

std::int64_t CPU::read(const Operand& op, ExecRecord& rec) {
    switch (op.kind) {
        case OperandKind::Register: return std::int64_t(getReg(op.reg, rec));
        case OperandKind::Immediate:
            rec.immUsed = true;
            rec.immValue = op.imm;
            return op.imm;
        case OperandKind::Memory: return memRead(effectiveAddress(op), rec);
        case OperandKind::Label: return std::int64_t(addressOf(op.target));
        default: break;
    }
    throw SyntaxError("Instruction is missing an operand", currentLine());
}

void CPU::write(const Operand& op, std::int64_t v, ExecRecord& rec) {
    switch (op.kind) {
        case OperandKind::Register: setReg(op.reg, std::uint64_t(v), rec); return;
        case OperandKind::Memory: memWrite(effectiveAddress(op), v, rec); return;
        default: break;
    }
    throw SyntaxError("Destination must be a register or memory location", currentLine());
}

static void checkAddress(std::uint64_t addr, int line) {
    if (addr % 8 != 0)
        throw MemoryAccessError("Unaligned qword access at " + hex64(addr) + " (addresses must be multiples of 8)", line);
    if (addr < CPU::kDataBase || addr >= CPU::kStackTop)
        throw MemoryAccessError("Address " + hex64(addr) + " is outside data/stack memory (0x2000-0x7FF8)", line);
}

std::int64_t CPU::memRead(std::uint64_t addr, ExecRecord& rec) {
    checkAddress(addr, currentLine());
    std::int64_t v = peek(addr);
    rec.mem.push_back({addr, v, false});
    return v;
}

void CPU::memWrite(std::uint64_t addr, std::int64_t v, ExecRecord& rec) {
    checkAddress(addr, currentLine());
    mem_.put(addr, v);
    rec.mem.push_back({addr, v, true});
}

void CPU::jump(int target, ExecRecord& rec) {
    if (!prog_ || target < 0 || target > int(prog_->code.size()))
        throw MemoryAccessError("Jump target is outside the program", currentLine());
    nextPc_ = target;
    rec.branchTaken = true;
    rec.branchTarget = target;
}

void CPU::push(std::int64_t v, ExecRecord& rec) {
    std::uint64_t rsp = getReg(RSP, rec) - 8;
    if (rsp < kStackLimit) throw StackOverflowError(currentLine());
    memWrite(rsp, v, rec);
    setReg(RSP, rsp, rec);
}

std::int64_t CPU::pop(ExecRecord& rec) {
    std::uint64_t rsp = getReg(RSP, rec);
    if (rsp >= kStackTop) throw MemoryAccessError("Stack underflow - POP/RET on an empty stack", currentLine());
    std::int64_t v = memRead(rsp, rec);
    setReg(RSP, rsp + 8, rec);
    return v;
}

} // namespace vas::core
