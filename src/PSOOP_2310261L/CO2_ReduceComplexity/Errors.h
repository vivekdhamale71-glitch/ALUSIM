// =====================================================================
//  Exception hierarchy                                       [PSOOP]
//  Every failure the simulator can hit is a typed exception, so the UI
//  can catch it and show a clear message instead of crashing.
// =====================================================================
#pragma once
#include <stdexcept>
#include <string>

namespace vas::core {

class SimulatorError : public std::runtime_error {
public:
    SimulatorError(std::string kind, const std::string& msg, int line = -1)
        : std::runtime_error(msg), kind_(std::move(kind)), line_(line) {}
    const std::string& kind() const { return kind_; }
    int line() const { return line_; }   // 0-based source line, -1 if unknown

private:
    std::string kind_;
    int line_;
};

class SyntaxError : public SimulatorError {
public:
    explicit SyntaxError(const std::string& m, int line = -1) : SimulatorError("Syntax Error", m, line) {}
};

class InvalidOpcodeError : public SimulatorError {
public:
    explicit InvalidOpcodeError(const std::string& m, int line = -1) : SimulatorError("Invalid Opcode", m, line) {}
};

class DivideByZeroError : public SimulatorError {
public:
    explicit DivideByZeroError(int line = -1)
        : SimulatorError("Divide Error (#DE)", "Division by zero - the CPU raised a divide exception", line) {}
};

class MemoryAccessError : public SimulatorError {
public:
    explicit MemoryAccessError(const std::string& m, int line = -1) : SimulatorError("Memory Fault", m, line) {}
};

class StackOverflowError : public SimulatorError {
public:
    explicit StackOverflowError(int line = -1)
        : SimulatorError("Stack Overflow", "RSP went below the stack limit (too many PUSH/CALL)", line) {}
};

class RuntimeLimitError : public SimulatorError {
public:
    explicit RuntimeLimitError(long steps)
        : SimulatorError("Step Limit", "Stopped after " + std::to_string(steps) +
                                           " instructions - is there an infinite loop?") {}
};

} // namespace vas::core
