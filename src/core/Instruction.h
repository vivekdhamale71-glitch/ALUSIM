// =====================================================================
//  Instruction set                                    [COA CO1, CO2]
//  An abstract Instruction with one derived class per instruction
//  group. The CPU only ever calls the virtual interface, so adding an
//  instruction never touches the CPU.            [PSOOP: polymorphism]
//  OpcodeTable is sorted once with quickSort; every mnemonic is then
//  found with binary search, and the entry it finds carries the
//  creator that InstructionFactory calls                    [PL CO4]
//  InstructionFactory: mnemonic -> the right Instruction subclass
//                                                    [PSOOP CO4 Factory]
// =====================================================================
#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "engine/ds/HashMap.h"
#include "engine/ds/HashTable.h"
#include "core/Register64.h"

namespace vas::core {

class CPU;

enum Reg { RAX, RBX, RCX, RDX, RSI, RDI, RSP, RBP, R8, R9, R10, R11, R12, R13, R14, R15, REG_COUNT };
const char* regName(int r);

enum class OperandKind { None, Register, Immediate, Memory, Label };

struct Operand {
    OperandKind kind = OperandKind::None;
    int reg = -1;              // register, or memory base register (-1 = none)
    std::int64_t imm = 0;      // immediate value, or memory displacement
    std::string symbol;        // memory symbol name / label name
    std::uint64_t symbolAddr = 0;
    int target = -1;           // label resolved to an instruction index
    bool isCL = false;         // the "cl" shift-count register
    std::string text;
};

enum class InstrClass { DataTransfer, Arithmetic, Logic, ControlFlow, Stack, System };
constexpr int kInstrClassCount = 6;
const char* instrClassName(InstrClass c);

// The control-signal vector produced by the control unit.  [COA CO4]
struct ControlSignals {
    bool PCWrite = false, IRWrite = false, MemRead = false, MemWrite = false, RegWrite = false;
    bool ALUSrc = false, MemToReg = false, Branch = false, Jump = false, FlagWrite = false, StackOp = false;
    AluOp aluOp = AluOp::None;

    static constexpr int kCount = 11;
    bool get(int i) const {
        const bool v[kCount] = {PCWrite, IRWrite, MemRead, MemWrite, RegWrite, ALUSrc,
                                MemToReg, Branch, Jump, FlagWrite, StackOp};
        return v[i];
    }
    static const char* name(int i) {
        static const char* n[kCount] = {"PCWrite", "IRWrite", "MemRead", "MemWrite", "RegWrite", "ALUSrc",
                                        "MemToReg", "Branch", "Jump", "FlagWrite", "StackOp"};
        return n[i];
    }
    ControlSignals operator|(const ControlSignals& o) const {
        ControlSignals r;
        r.PCWrite = PCWrite || o.PCWrite; r.IRWrite = IRWrite || o.IRWrite;
        r.MemRead = MemRead || o.MemRead; r.MemWrite = MemWrite || o.MemWrite;
        r.RegWrite = RegWrite || o.RegWrite; r.ALUSrc = ALUSrc || o.ALUSrc;
        r.MemToReg = MemToReg || o.MemToReg; r.Branch = Branch || o.Branch;
        r.Jump = Jump || o.Jump; r.FlagWrite = FlagWrite || o.FlagWrite; r.StackOp = StackOp || o.StackOp;
        r.aluOp = aluOp != AluOp::None ? aluOp : o.aluOp;
        return r;
    }
};

struct MemAccess {
    std::uint64_t addr;
    std::int64_t value;
    bool write;
};

// Everything that happened while one instruction executed. The datapath
// animation, the waveform, the pipeline and the trace log are all
// driven from these records.
struct ExecRecord {
    int index = -1;
    std::uint64_t pcAddr = 0;
    int nextIndex = -1;
    std::string text;
    std::string mnemonic;
    InstrClass klass = InstrClass::System;
    ControlSignals sig;

    bool aluUsed = false;
    AluOp aluOp = AluOp::None;
    std::uint64_t aluA = 0, aluB = 0, aluOut = 0;
    bool immUsed = false;
    std::int64_t immValue = 0;

    Flags flagsBefore, flagsAfter;
    std::uint64_t regBefore[REG_COUNT] = {};
    std::uint64_t regAfter[REG_COUNT] = {};
    std::vector<int> regsRead, regsWritten;
    std::vector<MemAccess> mem;

    bool branchTaken = false;
    int branchTarget = -1;
    bool halted = false;
    long stepNo = 0;
};

class Instruction {
public:
    Instruction(std::string mnem, std::vector<Operand> ops, int line, std::string text)
        : mnem_(std::move(mnem)), ops_(std::move(ops)), line_(line), text_(std::move(text)) {}
    virtual ~Instruction() = default;

    virtual void execute(CPU& cpu, ExecRecord& rec) const = 0;
    virtual InstrClass klass() const = 0;
    virtual ControlSignals signals() const = 0;            // decode of the execute stage
    virtual std::vector<int> readRegs() const;             // for hazard detection
    virtual std::vector<int> writeRegs() const;

    bool readsMemory() const;
    bool writesMemory() const;
    bool isBranch() const { return klass() == InstrClass::ControlFlow; }

    const std::string& mnemonic() const { return mnem_; }
    const std::vector<Operand>& operands() const { return ops_; }
    std::vector<Operand>& operands() { return ops_; }
    int line() const { return line_; }
    const std::string& text() const { return text_; }

    // static members: one copy shared by every instruction object  [PSOOP CO4]
    static long executedCount;                       // all executions since reset
    static long classCount[kInstrClassCount];        // executions per class (instruction mix)
    static void countExecution(InstrClass c);
    static void resetCounts();

protected:
    std::string mnem_;
    std::vector<Operand> ops_;
    int line_;
    std::string text_;
};

class DataTransferInstr : public Instruction {
public:
    using Instruction::Instruction;
    void execute(CPU& cpu, ExecRecord& rec) const override;
    InstrClass klass() const override { return InstrClass::DataTransfer; }
    ControlSignals signals() const override;
};

class ArithmeticInstr : public Instruction {
public:
    using Instruction::Instruction;
    void execute(CPU& cpu, ExecRecord& rec) const override;
    InstrClass klass() const override { return InstrClass::Arithmetic; }
    ControlSignals signals() const override;
    std::vector<int> readRegs() const override;
    std::vector<int> writeRegs() const override;
};

class LogicInstr : public Instruction {
public:
    using Instruction::Instruction;
    void execute(CPU& cpu, ExecRecord& rec) const override;
    InstrClass klass() const override { return InstrClass::Logic; }
    ControlSignals signals() const override;
    std::vector<int> readRegs() const override;
    std::vector<int> writeRegs() const override;
};

class ControlFlowInstr : public Instruction {
public:
    using Instruction::Instruction;
    void execute(CPU& cpu, ExecRecord& rec) const override;
    InstrClass klass() const override { return InstrClass::ControlFlow; }
    ControlSignals signals() const override;
    std::vector<int> readRegs() const override;
    std::vector<int> writeRegs() const override;
    bool conditionMet(const Flags& f) const;
};

class StackInstr : public Instruction {
public:
    using Instruction::Instruction;
    void execute(CPU& cpu, ExecRecord& rec) const override;
    InstrClass klass() const override { return InstrClass::Stack; }
    ControlSignals signals() const override;
    std::vector<int> readRegs() const override;
    std::vector<int> writeRegs() const override;
};

class SystemInstr : public Instruction {
public:
    using Instruction::Instruction;
    void execute(CPU& cpu, ExecRecord& rec) const override;
    InstrClass klass() const override { return InstrClass::System; }
    ControlSignals signals() const override { return {}; }
    std::vector<int> readRegs() const override { return {}; }
    std::vector<int> writeRegs() const override { return {}; }
};

// ------------------------------------------------------------ opcode table
using InstrCreator = std::unique_ptr<Instruction> (*)(const std::string&, std::vector<Operand>, int,
                                                     const std::string&);

struct OpcodeInfo {
    std::string mnemonic;
    InstrClass klass;
    int minOps, maxOps;
    std::string encoding;      // x86 primary opcode, for display
    std::string description;
    InstrCreator create = nullptr;   // builds the subclass for this mnemonic
};

class OpcodeTable {
public:
    static const OpcodeTable& instance();
    // binary search; `probes` receives the visited indices
    const OpcodeInfo* lookup(const std::string& mnem, std::vector<int>* probes = nullptr) const;
    const std::vector<OpcodeInfo>& sorted() const { return table_; }

private:
    OpcodeTable();
    std::vector<OpcodeInfo> table_;
};

// ----------------------------------------------------------------- factory
class InstructionFactory {
public:
    static const InstructionFactory& instance();
    // from an entry already found by binary search (the assembler's path)
    std::unique_ptr<Instruction> create(const OpcodeInfo& info, std::vector<Operand> ops, int line,
                                        const std::string& text) const;
    // looks the mnemonic up first (binary search in OpcodeTable)
    std::unique_ptr<Instruction> create(const std::string& mnem, std::vector<Operand> ops, int line,
                                        const std::string& text) const;

private:
    InstructionFactory() = default;
};

// ----------------------------------------------------------------- program
struct DataSymbol {
    std::string name;
    std::uint64_t addr;
    std::int64_t init;
};

struct OpcodeLookup {
    std::string mnemonic;
    std::vector<int> probes;
};

struct Program {
    std::vector<std::unique_ptr<Instruction>> code;
    std::vector<DataSymbol> data;
    std::vector<std::string> lines;          // assembly source lines
    std::vector<int> instrLine;              // instruction index -> source line
    std::vector<std::pair<std::string, int>> labels;
    std::vector<ds::ProbeTrace> labelTraces;
    std::vector<OpcodeLookup> opcodeLookups;
    std::unique_ptr<ds::HashTable> labelTable;

    int lineToInstr(int line) const {
        for (std::size_t i = 0; i < instrLine.size(); ++i) if (instrLine[i] == line) return int(i);
        return -1;
    }
};

} // namespace vas::core
