// =====================================================================
//  Debugger  -  friend class of CPU                        [PSOOP CO4]
//  CPU declares "friend class Debugger;", so these functions may use
//  its private register file, RIP and flags directly.
//   * setRegister - the register editor (click a selected register while
//     paused). Instructions change registers only through CPU::setReg,
//     which records every write in the ExecRecord (the trace, the
//     animation, Step Back). An edit by the user is a write from OUTSIDE
//     the program: it must not appear as a program step, and no
//     instruction may be able to make it - so it is not a public CPU
//     method, only this one trusted class may do it.
//   * dumpRegisters / describe - File > Export trace (Ctrl+E).
// =====================================================================
#pragma once
#include <cstdint>
#include <string>

namespace vas::core {

class CPU;
struct ExecRecord;

class Debugger {
public:
    static std::string dumpRegisters(const CPU& cpu);     // final machine state
    static std::string describe(const ExecRecord& rec);   // one trace line
    static void setRegister(CPU& cpu, int reg, std::uint64_t value);   // out-of-band write
};

} // namespace vas::core
