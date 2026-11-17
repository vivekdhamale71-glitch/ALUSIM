// =====================================================================
//  CPU  -  the virtual x86-64 processor                [COA CO1, CO2]
//  16 x 64-bit general registers, RIP, RFLAGS and a sparse memory.
//  Observers are notified after every instruction (profiler, trace
//  log)                                    [PSOOP: Observer pattern]
//  Debugger is a friend class that can read private state.   [PSOOP]
// =====================================================================
#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "PL_2310214L/CO1_Arrays/FixedArray.h"
#include "PL_2310214L/CO5_Hashing/HashMap.h"
#include "PSOOP_2310261L/CO2_ReduceComplexity/Errors.h"
#include "core/Instruction.h"
#include "core/Register64.h"

namespace vas::core {

class CPUObserver;   // Observer interface: PSOOP_2310261L/CO4_LogicBuilding/Profiler.h

using Memory = ds::HashMap<std::uint64_t, std::int64_t>;   // sparse qword memory

struct Snapshot {
    std::uint64_t regs[REG_COUNT] = {};
    Flags flags;
    int pc = 0;
    bool halted = false;
    long steps = 0;
    Memory mem;
};

class CPU {
    friend class Debugger;

public:
    static constexpr std::uint64_t kCodeBase = 0x1000;
    static constexpr std::uint64_t kDataBase = 0x2000;
    static constexpr std::uint64_t kStackTop = 0x8000;
    static constexpr std::uint64_t kStackLimit = 0x7000;
    static constexpr long kMaxSteps = 20000;

    CPU();

    void load(const Program* prog);
    void reset();
    ExecRecord step();                       // throws SimulatorError

    bool halted() const { return halted_; }
    bool loaded() const { return prog_ != nullptr; }
    int pc() const { return pc_; }
    long steps() const { return steps_; }
    const Program* program() const { return prog_; }

    Register64 reg(int r) const { return regs_[r]; }
    const Flags& flags() const { return flags_; }
    std::int64_t peek(std::uint64_t addr) const;
    const Memory& memory() const { return mem_; }

    Snapshot snapshot() const;
    void restore(const Snapshot& s);

    void addObserver(CPUObserver* o) { observers_.push_back(o); }

    static std::uint64_t addressOf(int index) { return kCodeBase + std::uint64_t(index) * 4; }
    static int indexOf(std::uint64_t addr) { return int((addr - kCodeBase) / 4); }

    // ---- used by Instruction::execute -------------------------------------
    std::int64_t read(const Operand& op, ExecRecord& rec);
    void write(const Operand& op, std::int64_t v, ExecRecord& rec);
    void setReg(int r, std::uint64_t v, ExecRecord& rec);
    std::uint64_t getReg(int r, ExecRecord& rec);
    void setFlags(const Flags& f) { flags_ = f; }
    void jump(int target, ExecRecord& rec);
    void push(std::int64_t v, ExecRecord& rec);
    std::int64_t pop(ExecRecord& rec);
    void halt() { halted_ = true; }
    std::uint64_t effectiveAddress(const Operand& op) const;
    std::int64_t memRead(std::uint64_t addr, ExecRecord& rec);
    void memWrite(std::uint64_t addr, std::int64_t v, ExecRecord& rec);
    int currentLine() const;

private:
    ds::FixedArray<Register64, REG_COUNT> regs_;   // the register file            [PL CO1]
    Flags flags_;
    int pc_ = 0;
    int nextPc_ = 0;
    bool halted_ = false;
    long steps_ = 0;
    Memory mem_;
    const Program* prog_ = nullptr;
    std::vector<CPUObserver*> observers_;
};

} // namespace vas::core

// Debugger (friend), Profiler (Observer) and Clock (Singleton)    [PSOOP CO4]
#include "PSOOP_2310261L/CO4_LogicBuilding/Patterns.h"
