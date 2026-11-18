// Instructions that run through one base class, and two control units that share another base class.

#include <string>
#include <vector>

// The machine that instructions work on: three registers and a program counter.
struct Machine {
    long long registers[3];
    int programCounter;
};

// The base class. The CPU only knows this interface, never the concrete instructions.
class Instruction {
public:
    virtual ~Instruction() {
    }

    // each kind of instruction decides for itself what it does (a virtual function)
    virtual void execute(Machine& machine) const = 0;

    virtual std::string name() const = 0;
};

// mov register, value
class MoveInstruction : public Instruction {
public:
    MoveInstruction(int reg, long long value) : reg_(reg), value_(value) {
    }

    void execute(Machine& machine) const override {
        machine.registers[reg_] = value_;
        machine.programCounter = machine.programCounter + 1;
    }

    std::string name() const override {
        return "mov";
    }

private:
    int reg_;
    long long value_;
};

// add register, register
class AddInstruction : public Instruction {
public:
    AddInstruction(int destination, int source) : destination_(destination), source_(source) {
    }

    void execute(Machine& machine) const override {
        machine.registers[destination_] = machine.registers[destination_] + machine.registers[source_];
        machine.programCounter = machine.programCounter + 1;
    }

    std::string name() const override {
        return "add";
    }

private:
    int destination_;
    int source_;
};

// jump to another instruction number
class JumpInstruction : public Instruction {
public:
    explicit JumpInstruction(int target) : target_(target) {
    }

    void execute(Machine& machine) const override {
        machine.programCounter = target_;
    }

    std::string name() const override {
        return "jmp";
    }

private:
    int target_;
};

// The CPU: it runs a program through the base-class pointer. Adding a new kind of
// instruction never changes this function.
void runProgram(const std::vector<Instruction*>& program, Machine& machine, int maxSteps) {
    machine.programCounter = 0;
    int steps = 0;
    while (machine.programCounter < static_cast<int>(program.size()) && steps < maxSteps) {
        program[machine.programCounter]->execute(machine);     // the right version runs
        steps = steps + 1;
    }
}

// ---------------------------------------------------------------------
// The same idea for the control units: one interface, two ways of working.
// ---------------------------------------------------------------------
class ControlUnit {
public:
    virtual ~ControlUnit() {
    }

    // how many clock cycles one instruction takes to decode and control
    virtual int cyclesFor(const Instruction& instruction) const = 0;

    virtual std::string name() const = 0;
};

class HardwiredControlUnit : public ControlUnit {
public:
    int cyclesFor(const Instruction&) const override {
        return 1;                                // fixed logic answers at once
    }

    std::string name() const override {
        return "hardwired";
    }
};

class MicroprogrammedControlUnit : public ControlUnit {
public:
    int cyclesFor(const Instruction& instruction) const override {
        if (instruction.name() == "jmp") {
            return 3;                            // a short micro-routine
        }
        return 5;                                // fetch micro-steps plus the routine
    }

    std::string name() const override {
        return "micro-programmed";
    }
};

// total cycles of a program for ANY control unit: only the base class is used
int totalCycles(const std::vector<Instruction*>& program, const ControlUnit& unit) {
    int total = 0;
    for (std::size_t i = 0; i < program.size(); ++i) {
        total = total + unit.cyclesFor(*program[i]);
    }
    return total;
}
