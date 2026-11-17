#include "PSOOP_2310261L/CO4_LogicBuilding/Profiler.h"

#include "core/CPU.h"

namespace vas::core {

void Profiler::onStep(const ExecRecord& rec) {
    int* c = counts_.find(rec.mnemonic);
    if (c) {
        ++*c;
    } else {
        counts_.put(rec.mnemonic, 1);
        order_.push_back(rec.mnemonic);
    }
}

void Profiler::onReset() {
    counts_.clear();
    order_.clear();
}

std::vector<std::pair<std::string, int>> Profiler::counts() const {
    std::vector<std::pair<std::string, int>> v;
    for (const auto& m : order_) v.push_back({m, *counts_.find(m)});
    return v;
}

} // namespace vas::core
