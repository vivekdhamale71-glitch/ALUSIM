// Error classes derived from one base class and caught in a single place.

#include <stdexcept>
#include <string>

// The base of every error the machine can raise. It remembers a short kind and the source line.
class SimulatorError : public std::runtime_error {
public:
    SimulatorError(const std::string& kind, const std::string& message, int line)
        : std::runtime_error(message), kind_(kind), line_(line) {
    }

    const std::string& kind() const {
        return kind_;
    }

    int line() const {
        return line_;
    }

private:
    std::string kind_;
    int line_;
};

// a sentence or statement that cannot be understood
class SyntaxError : public SimulatorError {
public:
    SyntaxError(const std::string& message, int line) : SimulatorError("Syntax Error", message, line) {
    }
};

// IDIV by zero
class DivideByZeroError : public SimulatorError {
public:
    explicit DivideByZeroError(int line) : SimulatorError("Divide Error", "division by zero", line) {
    }
};

// an address outside the memory that exists
class MemoryAccessError : public SimulatorError {
public:
    MemoryAccessError(const std::string& message, int line) : SimulatorError("Memory Fault", message, line) {
    }
};

// RSP went below the stack limit
class StackOverflowError : public SimulatorError {
public:
    explicit StackOverflowError(int line) : SimulatorError("Stack Overflow", "too many PUSH or CALL", line) {
    }
};

// ---------------------------------------------------------------------
// Code that raises the errors says only WHAT went wrong ...
// ---------------------------------------------------------------------
long long divide(long long dividend, long long divisor, int line) {
    if (divisor == 0) {
        throw DivideByZeroError(line);
    }
    return dividend / divisor;
}

long long readWord(const long long memory[], long long size, long long address, int line) {
    if (address < 0 || address >= size) {
        throw MemoryAccessError("address " + std::to_string(address) + " is outside the memory", line);
    }
    return memory[address];
}

// ---------------------------------------------------------------------
// ... and ONE place deals with all of them, because they share a base class.
// Catching by reference keeps the real type (no slicing).
// ---------------------------------------------------------------------
std::string runAndReport(long long dividend, long long divisor, int line) {
    try {
        long long result = divide(dividend, divisor, line);
        return "result " + std::to_string(result);
    } catch (const SimulatorError& error) {
        return error.kind() + " on line " + std::to_string(error.line()) + ": " + error.what();
    }
}
