// =====================================================================
//  Exception hierarchy                                 [PSOOP CO2, CO3]
//  Every failure the simulator can hit is a typed exception derived
//  from SimulatorError (itself a std::runtime_error). Throwing code
//  only says WHAT went wrong; one catch (const SimulatorError&) in
//  app/Sim.cpp turns any of them into an error toast with the line.
//
//      std::runtime_error
//        +-- SimulatorError       kind() + line()
//              +-- SyntaxError         assembler / compiler / English
//              +-- InvalidOpcodeError  unknown mnemonic
//              +-- DivideByZeroError   IDIV by 0
//              +-- MemoryAccessError   bad address, stack underflow
//              +-- StackOverflowError  RSP below the stack limit
//              +-- RuntimeLimitError   probable infinite loop
// =====================================================================
#pragma once
#include <stdexcept>
#include <string>

namespace vas::core {

class SimulatorError : public std::runtime_error {
public:
    SimulatorError(std::string kind, const std::string& msg, int line = -1);
    const std::string& kind() const { return kind_; }
    int line() const { return line_; }   // 0-based source line, -1 if unknown

private:
    std::string kind_;
    int line_;
};

class SyntaxError : public SimulatorError {
public:
    explicit SyntaxError(const std::string& m, int line = -1);
};

class InvalidOpcodeError : public SimulatorError {
public:
    explicit InvalidOpcodeError(const std::string& m, int line = -1);
};

class DivideByZeroError : public SimulatorError {
public:
    explicit DivideByZeroError(int line = -1);
};

class MemoryAccessError : public SimulatorError {
public:
    explicit MemoryAccessError(const std::string& m, int line = -1);
};

class StackOverflowError : public SimulatorError {
public:
    explicit StackOverflowError(int line = -1);
};

class RuntimeLimitError : public SimulatorError {
public:
    explicit RuntimeLimitError(long steps);
};

} // namespace vas::core
