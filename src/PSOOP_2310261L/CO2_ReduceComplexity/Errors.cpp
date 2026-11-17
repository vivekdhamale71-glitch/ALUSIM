#include "PSOOP_2310261L/CO2_ReduceComplexity/Errors.h"

#include <utility>

namespace vas::core {

// each subclass only fixes its kind and message; the base stores them
SimulatorError::SimulatorError(std::string kind, const std::string& msg, int line)
    : std::runtime_error(msg), kind_(std::move(kind)), line_(line) {}

SyntaxError::SyntaxError(const std::string& m, int line) : SimulatorError("Syntax Error", m, line) {}

InvalidOpcodeError::InvalidOpcodeError(const std::string& m, int line) : SimulatorError("Invalid Opcode", m, line) {}

DivideByZeroError::DivideByZeroError(int line)
    : SimulatorError("Divide Error (#DE)", "Division by zero - the CPU raised a divide exception", line) {}

MemoryAccessError::MemoryAccessError(const std::string& m, int line) : SimulatorError("Memory Fault", m, line) {}

StackOverflowError::StackOverflowError(int line)
    : SimulatorError("Stack Overflow", "RSP went below the stack limit (too many PUSH/CALL)", line) {}

RuntimeLimitError::RuntimeLimitError(long steps)
    : SimulatorError("Step Limit", "Stopped after " + std::to_string(steps) + " instructions - is there an infinite loop?") {}

} // namespace vas::core
