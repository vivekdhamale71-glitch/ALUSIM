// =====================================================================
//  Design patterns used around the CPU                     [PSOOP CO4]
//   * Debugger  - a FRIEND class of CPU: reads its private registers to
//                 export the execution trace (Ctrl+E)
//   * Profiler  - OBSERVER: subscribes to the CPU and counts how often
//                 each instruction runs (feeds the sorting demo)
//   * Clock     - SINGLETON: one global cycle counter for the whole app
//  Together with the Factory (InstructionFactory), Strategy (control
//  units, hash methods) and Command/undo (Step Back history) these are
//  the logic-building patterns of the project.
// =====================================================================
#pragma once
#include <string>
#include <utility>
#include <vector>

#include "COA_2310221L/CO1_Architecture/CPU.h"

namespace vas::core {

class Debugger {
public:
    static std::string dumpRegisters(const CPU& cpu);
    static std::string describe(const ExecRecord& rec);
};

class Profiler : public CPUObserver {
public:
    void onStep(const ExecRecord& rec) override;
    void onReset() override { counts_.clear(); order_.clear(); }
    std::vector<std::pair<std::string, int>> counts() const;

private:
    ds::HashMap<std::string, int> counts_;
    std::vector<std::string> order_;
};

class Clock {
public:
    static Clock& instance() {
        static Clock c;
        return c;
    }
    void tick() { ++cycle_; }
    void reset() { cycle_ = 0; }
    long cycle() const { return cycle_; }
    Clock(const Clock&) = delete;
    Clock& operator=(const Clock&) = delete;

private:
    Clock() = default;
    long cycle_ = 0;
};

} // namespace vas::core
