// Control units that can be switched at run time (strategy) and run states that change on events (state).

#include <string>

// ---------------------------------------------------------------------
// Strategy: interchangeable ways of doing the same job behind one interface
// ---------------------------------------------------------------------
class ControlUnit {
public:
    virtual ~ControlUnit() {
    }

    // clock cycles needed to produce the control signals of one instruction
    virtual int cyclesFor(const std::string& mnemonic) const = 0;
    virtual std::string name() const = 0;
};

// fixed logic answers in one step
class HardwiredUnit : public ControlUnit {
public:
    int cyclesFor(const std::string&) const override {
        return 1;
    }

    std::string name() const override {
        return "hardwired";
    }
};

// stored micro-instructions: the cost depends on the length of the micro-routine
class MicroprogrammedUnit : public ControlUnit {
public:
    int cyclesFor(const std::string& mnemonic) const override {
        if (mnemonic == "jmp" || mnemonic == "nop") {
            return 3;
        }
        if (mnemonic == "idiv") {
            return 8;
        }
        return 5;
    }

    std::string name() const override {
        return "micro-programmed";
    }
};

// The user of the strategy only holds a pointer to the base class, so the strategy can
// be switched while it runs.
class Machine {
public:
    Machine() : unit_(nullptr), cycles_(0) {
    }

    void useControlUnit(ControlUnit* unit) {
        unit_ = unit;
    }

    void execute(const std::string& mnemonic) {
        cycles_ = cycles_ + unit_->cyclesFor(mnemonic);
    }

    int cycles() const {
        return cycles_;
    }

private:
    ControlUnit* unit_;
    int cycles_;
};

// ---------------------------------------------------------------------
// State: the behaviour depends on which state the machine is in
// ---------------------------------------------------------------------
class RunState {
public:
    virtual ~RunState() {
    }

    virtual std::string name() const = 0;
    virtual RunState* onRun() = 0;          // what happens when the user presses Run
    virtual RunState* onPause() = 0;
    virtual RunState* onHalt() = 0;
};

class Idle : public RunState {
public:
    std::string name() const override {
        return "idle";
    }
    RunState* onRun() override;
    RunState* onPause() override {
        return this;                        // nothing to pause
    }
    RunState* onHalt() override;
};

class Running : public RunState {
public:
    std::string name() const override {
        return "running";
    }
    RunState* onRun() override {
        return this;                        // already running
    }
    RunState* onPause() override;
    RunState* onHalt() override;
};

class Paused : public RunState {
public:
    std::string name() const override {
        return "paused";
    }
    RunState* onRun() override;
    RunState* onPause() override {
        return this;
    }
    RunState* onHalt() override;
};

class Halted : public RunState {
public:
    std::string name() const override {
        return "halted";
    }
    RunState* onRun() override {
        return this;                        // a halted program cannot run again until it is reset
    }
    RunState* onPause() override {
        return this;
    }
    RunState* onHalt() override {
        return this;
    }
};

// one object of each state; a transition just hands back another one
Idle idleState;
Running runningState;
Paused pausedState;
Halted haltedState;

RunState* Idle::onRun() {
    return &runningState;
}
RunState* Idle::onHalt() {
    return &haltedState;
}
RunState* Running::onPause() {
    return &pausedState;
}
RunState* Running::onHalt() {
    return &haltedState;
}
RunState* Paused::onRun() {
    return &runningState;
}
RunState* Paused::onHalt() {
    return &haltedState;
}
