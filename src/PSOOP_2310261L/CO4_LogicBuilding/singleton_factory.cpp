// A singleton clock, a factory that builds instructions by name, a static counter and a friend class.

#include <string>

// ---------------------------------------------------------------------
// Singleton: exactly one object, reached through instance()
// ---------------------------------------------------------------------
class Clock {
public:
    // the only way to get the clock; it is created the first time it is asked for
    static Clock& instance() {
        static Clock theClock;
        return theClock;
    }

    void tick() {
        cycle_ = cycle_ + 1;
    }

    void reset() {
        cycle_ = 0;
    }

    long cycle() const {
        return cycle_;
    }

    // copying would create a second clock, so it is forbidden
    Clock(const Clock&) = delete;
    Clock& operator=(const Clock&) = delete;

private:
    Clock() : cycle_(0) {
    }

    long cycle_;
};

// ---------------------------------------------------------------------
// Factory: builds the right kind of object from a name
// ---------------------------------------------------------------------
class Instruction {
public:
    Instruction() {
        created_ = created_ + 1;
    }

    virtual ~Instruction() {
    }

    virtual std::string describe() const = 0;

    // a static member: ONE counter shared by every instruction object
    static int created() {
        return created_;
    }

private:
    static int created_;
};

int Instruction::created_ = 0;

class MoveInstruction : public Instruction {
public:
    std::string describe() const override {
        return "copy a value";
    }
};

class AddInstruction : public Instruction {
public:
    std::string describe() const override {
        return "add two values";
    }
};

class JumpInstruction : public Instruction {
public:
    std::string describe() const override {
        return "continue somewhere else";
    }
};

// the caller asks for "add" and gets an AddInstruction without knowing its class
Instruction* makeInstruction(const std::string& mnemonic) {
    if (mnemonic == "mov") {
        return new MoveInstruction();
    }
    if (mnemonic == "add") {
        return new AddInstruction();
    }
    if (mnemonic == "jmp") {
        return new JumpInstruction();
    }
    return nullptr;
}

// ---------------------------------------------------------------------
// Friend class: one trusted class may use another class's private data
// ---------------------------------------------------------------------
class Cpu {
public:
    Cpu() : rax_(0) {
    }

    friend class Debugger;

private:
    long long rax_;
};

class Debugger {
public:
    // writes a register without going through the CPU's normal rules (an edit by the user)
    static void setRax(Cpu& cpu, long long value) {
        cpu.rax_ = value;
    }

    static long long readRax(const Cpu& cpu) {
        return cpu.rax_;
    }
};
