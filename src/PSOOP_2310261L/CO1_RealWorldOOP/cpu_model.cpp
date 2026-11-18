// The virtual processor as a class: private registers, flags, program counter and memory behind a small public interface.

#include <cstdint>
#include <map>
#include <string>
#include <vector>

const int REGISTER_COUNT = 16;

// the four status flags set by the ALU
struct Flags {
    bool carry = false;
    bool zero = false;
    bool sign = false;
    bool overflow = false;
};

class CPU;

// One instruction of the program. The CPU only knows this interface (see CO2 for the full hierarchy).
class Instruction {
public:
    virtual ~Instruction() {
    }
    virtual void execute(CPU& cpu) const = 0;
};

// A copy of everything the CPU holds, so that a step can be undone later.
struct Snapshot {
    std::uint64_t registers[REGISTER_COUNT];
    Flags flags;
    int programCounter;
    bool halted;
    std::map<std::uint64_t, std::int64_t> memory;
};

// The processor. All of its state is private: an instruction can change it only through the
// member functions below, so every change goes through one place (encapsulation).
class CPU {
public:
    // addresses of the memory map: shared by all CPU objects, so they are static constants
    static const std::uint64_t DATA_BASE = 0x2000;
    static const std::uint64_t STACK_TOP = 0x8000;
    static const std::uint64_t STACK_LIMIT = 0x7000;

    // a new CPU starts in the reset state
    CPU() {
        reset();
    }

    // puts every register, the flags, the counter and the memory back to the start state
    void reset() {
        for (int i = 0; i < REGISTER_COUNT; ++i) {
            registers_[i] = 0;
        }
        registers_[STACK_POINTER] = STACK_TOP;
        flags_ = Flags();
        programCounter_ = 0;
        halted_ = false;
        steps_ = 0;
        memory_.clear();
    }

    // runs the instruction the program counter points to; returns false when the machine is halted
    bool step(const std::vector<Instruction*>& program) {
        if (halted_ || programCounter_ < 0 || programCounter_ >= static_cast<int>(program.size())) {
            return false;
        }
        nextCounter_ = programCounter_ + 1;
        program[programCounter_]->execute(*this);          // the instruction calls back into the CPU
        programCounter_ = nextCounter_;
        steps_ = steps_ + 1;
        return true;
    }

    // read-only access for the views: they may look, but not change
    std::uint64_t reg(int index) const {
        return registers_[index];
    }

    const Flags& flags() const {
        return flags_;
    }

    int programCounter() const {
        return programCounter_;
    }

    bool halted() const {
        return halted_;
    }

    long steps() const {
        return steps_;
    }

    // the functions an instruction uses to change the machine
    void setRegister(int index, std::uint64_t value) {
        registers_[index] = value;
    }

    void setFlags(const Flags& flags) {
        flags_ = flags;
    }

    // a jump only changes where the NEXT instruction is taken from
    void jump(int target) {
        nextCounter_ = target;
    }

    void halt() {
        halted_ = true;
    }

    // memory words that were never written read as 0
    std::int64_t readMemory(std::uint64_t address) const {
        std::map<std::uint64_t, std::int64_t>::const_iterator found = memory_.find(address);
        if (found == memory_.end()) {
            return 0;
        }
        return found->second;
    }

    void writeMemory(std::uint64_t address, std::int64_t value) {
        memory_[address] = value;
    }

    // pushes a value: the stack pointer goes down first, then the value is stored where it points
    bool push(std::int64_t value) {
        if (registers_[STACK_POINTER] <= STACK_LIMIT) {
            return false;                                  // stack overflow
        }
        registers_[STACK_POINTER] = registers_[STACK_POINTER] - 8;
        memory_[registers_[STACK_POINTER]] = value;
        return true;
    }

    // pops a value: read where the stack pointer points, then it goes up
    std::int64_t pop() {
        std::int64_t value = readMemory(registers_[STACK_POINTER]);
        registers_[STACK_POINTER] = registers_[STACK_POINTER] + 8;
        return value;
    }

    // a copy of the whole state, used by Step Back
    Snapshot snapshot() const {
        Snapshot copy;
        for (int i = 0; i < REGISTER_COUNT; ++i) {
            copy.registers[i] = registers_[i];
        }
        copy.flags = flags_;
        copy.programCounter = programCounter_;
        copy.halted = halted_;
        copy.memory = memory_;
        return copy;
    }

    // puts the machine back into a saved state
    void restore(const Snapshot& copy) {
        for (int i = 0; i < REGISTER_COUNT; ++i) {
            registers_[i] = copy.registers[i];
        }
        flags_ = copy.flags;
        programCounter_ = copy.programCounter;
        nextCounter_ = copy.programCounter;
        halted_ = copy.halted;
        memory_ = copy.memory;
    }

private:
    static const int STACK_POINTER = 6;                    // register number of rsp

    std::uint64_t registers_[REGISTER_COUNT];
    Flags flags_;
    int programCounter_;
    int nextCounter_;
    bool halted_;
    long steps_;
    std::map<std::uint64_t, std::int64_t> memory_;
};
