// Commands that can be done and undone, with an undo and redo history.

#include <string>
#include <vector>

// The thing the commands change: the registers of a machine.
struct Registers {
    long long values[4];
};

// The Command interface: an action that can be done and undone.
class Command {
public:
    virtual ~Command() {
    }

    virtual void execute(Registers& registers) = 0;
    virtual void undo(Registers& registers) = 0;
    virtual std::string describe() const = 0;
};

// "set register n to value": remembers the old value so that it can be put back
class SetRegisterCommand : public Command {
public:
    SetRegisterCommand(int index, long long value) : index_(index), newValue_(value), oldValue_(0) {
    }

    void execute(Registers& registers) override {
        oldValue_ = registers.values[index_];            // save what is being replaced
        registers.values[index_] = newValue_;
    }

    void undo(Registers& registers) override {
        registers.values[index_] = oldValue_;
    }

    std::string describe() const override {
        return "set r" + std::to_string(index_) + " = " + std::to_string(newValue_);
    }

private:
    int index_;
    long long newValue_;
    long long oldValue_;
};

// "add n to register": undone by subtracting it again
class AddToRegisterCommand : public Command {
public:
    AddToRegisterCommand(int index, long long amount) : index_(index), amount_(amount) {
    }

    void execute(Registers& registers) override {
        registers.values[index_] = registers.values[index_] + amount_;
    }

    void undo(Registers& registers) override {
        registers.values[index_] = registers.values[index_] - amount_;
    }

    std::string describe() const override {
        return "add " + std::to_string(amount_) + " to r" + std::to_string(index_);
    }

private:
    int index_;
    long long amount_;
};

// The history: two stacks of commands, one for undo and one for redo.
class CommandHistory {
public:
    ~CommandHistory() {
        for (std::size_t i = 0; i < done_.size(); ++i) {
            delete done_[i];
        }
        for (std::size_t i = 0; i < undone_.size(); ++i) {
            delete undone_[i];
        }
    }

    // does a new command; the redo stack is emptied because that future is gone
    void run(Command* command, Registers& registers) {
        command->execute(registers);
        done_.push_back(command);
        for (std::size_t i = 0; i < undone_.size(); ++i) {
            delete undone_[i];
        }
        undone_.clear();
    }

    bool undo(Registers& registers) {
        if (done_.empty()) {
            return false;
        }
        Command* command = done_.back();
        done_.pop_back();
        command->undo(registers);
        undone_.push_back(command);
        return true;
    }

    bool redo(Registers& registers) {
        if (undone_.empty()) {
            return false;
        }
        Command* command = undone_.back();
        undone_.pop_back();
        command->execute(registers);
        done_.push_back(command);
        return true;
    }

private:
    std::vector<Command*> done_;
    std::vector<Command*> undone_;
};
