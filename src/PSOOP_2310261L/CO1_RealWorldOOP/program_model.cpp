// A program as an object that owns its instructions, data variables and labels, built by an assembler class.

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

class Instruction {
public:
    virtual ~Instruction() {
    }
    virtual std::string text() const = 0;
};

// a plain instruction that only remembers its source text
class TextInstruction : public Instruction {
public:
    explicit TextInstruction(const std::string& text) : text_(text) {
    }

    std::string text() const override {
        return text_;
    }

private:
    std::string text_;
};

// a variable of the data section: its name, its address in memory and its starting value
struct DataSymbol {
    std::string name;
    std::uint64_t address;
    std::int64_t initial;
};

// The program owns its parts (composition): when the program is destroyed, they go with it.
class Program {
public:
    Program() {
    }

    // a program owns raw pointers, so copying it would delete the same instruction twice: copying is not allowed
    Program(const Program&) = delete;
    Program& operator=(const Program&) = delete;

    ~Program() {
        for (std::size_t i = 0; i < code_.size(); ++i) {
            delete code_[i];
        }
    }

    // the program takes ownership of the instruction
    void addInstruction(Instruction* instruction, int sourceLine) {
        code_.push_back(instruction);
        sourceLine_.push_back(sourceLine);
    }

    // a label names the number of the next instruction
    void addLabel(const std::string& name) {
        labels_.push_back(std::make_pair(name, static_cast<int>(code_.size())));
    }

    void addData(const std::string& name, std::int64_t initial) {
        DataSymbol symbol;
        symbol.name = name;
        symbol.address = DATA_BASE + 8 * data_.size();
        symbol.initial = initial;
        data_.push_back(symbol);
    }

    int instructionCount() const {
        return static_cast<int>(code_.size());
    }

    const Instruction& instruction(int index) const {
        return *code_[index];
    }

    // the instruction number of a label, or -1 if there is no such label
    int labelTarget(const std::string& name) const {
        for (std::size_t i = 0; i < labels_.size(); ++i) {
            if (labels_[i].first == name) {
                return labels_[i].second;
            }
        }
        return -1;
    }

    // the address of a variable, or 0 if there is no such variable
    std::uint64_t addressOf(const std::string& name) const {
        for (std::size_t i = 0; i < data_.size(); ++i) {
            if (data_[i].name == name) {
                return data_[i].address;
            }
        }
        return 0;
    }

    // which source line an instruction came from, used to highlight it in the editor
    int sourceLineOf(int index) const {
        return sourceLine_[index];
    }

    const std::vector<DataSymbol>& data() const {
        return data_;
    }

private:
    static const std::uint64_t DATA_BASE = 0x2000;

    std::vector<Instruction*> code_;
    std::vector<int> sourceLine_;
    std::vector<DataSymbol> data_;
    std::vector<std::pair<std::string, int> > labels_;
};

// The assembler is a separate class with one job: it reads source lines and fills a Program.
// Two passes: the first notes every label and variable, the second can then use them.
class Assembler {
public:
    // returns a new program that the caller owns, or nullptr when a line cannot be understood
    static Program* assemble(const std::vector<std::string>& lines) {
        Program* program = new Program();
        for (std::size_t i = 0; i < lines.size(); ++i) {
            std::string line = trim(lines[i]);
            if (line.empty()) {
                continue;
            }
            if (line[line.size() - 1] == ':') {
                program->addLabel(line.substr(0, line.size() - 1));
            }
        }
        for (std::size_t i = 0; i < lines.size(); ++i) {
            std::string line = trim(lines[i]);
            if (line.empty() || line[line.size() - 1] == ':') {
                continue;
            }
            if (line.compare(0, 3, "dq ") == 0) {
                program->addData("var" + std::to_string(program->data().size()), 0);
                continue;
            }
            program->addInstruction(new TextInstruction(line), static_cast<int>(i));
        }
        return program;
    }

private:
    // removes spaces at both ends of a line
    static std::string trim(const std::string& text) {
        std::size_t first = text.find_first_not_of(" \t");
        if (first == std::string::npos) {
            return "";
        }
        std::size_t last = text.find_last_not_of(" \t\r");
        return text.substr(first, last - first + 1);
    }
};
