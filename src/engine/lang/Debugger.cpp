#include "engine/lang/Debugger.h"

#include <cstdio>
#include <sstream>

#include "core/CPU.h"

namespace vas::core {

static std::string hex64(std::uint64_t v) {
    char b[32];
    std::snprintf(b, sizeof b, "0x%llX", static_cast<unsigned long long>(v));
    return b;
}

// regs_, pc_ and flags_ are private members of CPU: only a friend can read them
std::string Debugger::dumpRegisters(const CPU& cpu) {
    std::ostringstream os;
    for (int i = 0; i < REG_COUNT; ++i) {
        os << regName(i) << "=" << cpu.regs_[i] << (i % 4 == 3 ? "\n" : "  ");
    }
    os << "RIP=" << hex64(CPU::addressOf(cpu.pc_)) << "  CF=" << cpu.flags_.CF << " ZF=" << cpu.flags_.ZF
       << " SF=" << cpu.flags_.SF << " OF=" << cpu.flags_.OF << "\n";
    return os.str();
}

// writes the private register file directly: no ExecRecord, no trace entry
void Debugger::setRegister(CPU& cpu, int reg, std::uint64_t value) {
    if (reg < 0 || reg >= REG_COUNT) {
        return;
    }
    cpu.regs_[reg] = Register64(value);
}

std::string Debugger::describe(const ExecRecord& rec) {
    std::ostringstream os;
    char head[64];
    std::snprintf(head, sizeof head, "#%-4ld %s  ", rec.stepNo, hex64(rec.pcAddr).c_str());
    os << head << rec.text;
    for (int r : rec.regsWritten) {
        os << "   " << regName(r) << "<-" << hex64(rec.regAfter[r]);
    }
    for (const auto& m : rec.mem) {
        if (m.write) {
            os << "   [" << hex64(m.addr) << "]<-" << m.value;
        }
    }
    if (rec.branchTaken) {
        os << "   jump->#" << rec.branchTarget;
    }
    return os.str();
}

} // namespace vas::core
