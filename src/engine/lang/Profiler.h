// =====================================================================
//  Profiler  -  Observer of the CPU                        [PSOOP CO4]
//  The CPU (subject) keeps a list of CPUObserver*; after every
//  instruction it calls onStep() on each one. The Profiler counts how
//  often each mnemonic ran. The CPU does not know what observers do.
//  Registered in app/Sim.cpp (cpu.addObserver(&profiler)); its counts
//  are the data the Data Lab sorts.
// =====================================================================
#pragma once
#include <string>
#include <utility>
#include <vector>

#include "engine/ds/HashMap.h"

namespace vas::core {

struct ExecRecord;

class CPUObserver {
public:
    virtual ~CPUObserver() = default;
    virtual void onStep(const ExecRecord& rec) = 0;
    virtual void onReset() {}
};

class Profiler : public CPUObserver {
public:
    void onStep(const ExecRecord& rec) override;
    void onReset() override;
    std::vector<std::pair<std::string, int>> counts() const;   // in first-seen order

private:
    ds::HashMap<std::string, int> counts_;
    std::vector<std::string> order_;
};

} // namespace vas::core
