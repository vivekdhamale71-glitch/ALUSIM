// =====================================================================
//  Debugger  -  friend class of CPU                        [PSOOP CO4]
//  CPU declares "friend class Debugger;", so these functions may read
//  its private register file, RIP and flags without public getters.
//  Used by File > Export trace (Ctrl+E): app/Sim.cpp exportTrace().
// =====================================================================
#pragma once
#include <string>

namespace vas::core {

class CPU;
struct ExecRecord;

class Debugger {
public:
    static std::string dumpRegisters(const CPU& cpu);     // final machine state
    static std::string describe(const ExecRecord& rec);   // one trace line
};

} // namespace vas::core
