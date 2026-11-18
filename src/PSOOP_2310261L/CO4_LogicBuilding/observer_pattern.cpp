// A processor that informs its observers after every instruction, with two observers that count.

#include <map>
#include <string>
#include <vector>

// What an observer is told after every instruction.
struct StepInfo {
    std::string mnemonic;
    int memoryReads;
    int memoryWrites;
};

// The Observer interface: anything that wants to be told about a step implements it.
class StepObserver {
public:
    virtual ~StepObserver() {
    }

    virtual void onStep(const StepInfo& step) = 0;
    virtual void onReset() = 0;
};

// The subject: it keeps a list of observers and calls each one after a step. It does
// not know what the observers do with the information.
class Processor {
public:
    void addObserver(StepObserver* observer) {
        observers_.push_back(observer);
    }

    void removeObserver(StepObserver* observer) {
        for (std::size_t i = 0; i < observers_.size(); ++i) {
            if (observers_[i] == observer) {
                observers_.erase(observers_.begin() + static_cast<long>(i));
                return;
            }
        }
    }

    // runs one instruction and then tells everybody
    void step(const std::string& mnemonic, int reads, int writes) {
        StepInfo info;
        info.mnemonic = mnemonic;
        info.memoryReads = reads;
        info.memoryWrites = writes;
        for (std::size_t i = 0; i < observers_.size(); ++i) {
            observers_[i]->onStep(info);
        }
    }

    void reset() {
        for (std::size_t i = 0; i < observers_.size(); ++i) {
            observers_[i]->onReset();
        }
    }

private:
    std::vector<StepObserver*> observers_;
};

// Observer 1: counts how often every instruction ran (the Profiler).
class InstructionCounter : public StepObserver {
public:
    void onStep(const StepInfo& step) override {
        counts_[step.mnemonic] = counts_[step.mnemonic] + 1;
    }

    void onReset() override {
        counts_.clear();
    }

    int countOf(const std::string& mnemonic) const {
        std::map<std::string, int>::const_iterator found = counts_.find(mnemonic);
        if (found == counts_.end()) {
            return 0;
        }
        return found->second;
    }

private:
    std::map<std::string, int> counts_;
};

// Observer 2: counts memory traffic. Added without changing Processor at all.
class MemoryTrafficCounter : public StepObserver {
public:
    MemoryTrafficCounter() : reads_(0), writes_(0) {
    }

    void onStep(const StepInfo& step) override {
        reads_ = reads_ + step.memoryReads;
        writes_ = writes_ + step.memoryWrites;
    }

    void onReset() override {
        reads_ = 0;
        writes_ = 0;
    }

    int reads() const {
        return reads_;
    }

    int writes() const {
        return writes_;
    }

private:
    int reads_;
    int writes_;
};
