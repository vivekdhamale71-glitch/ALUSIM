// Turns a statement into assembly in three steps: lexer, shunting-yard parser and code generator.

#include <string>
#include <vector>

// The problem: turn "c = a + b * 2" into assembly. It is solved in three steps,
// each one simple on its own.

// Step 1: the lexer cuts the text into tokens (words, numbers and single symbols)
std::vector<std::string> tokenize(const std::string& text) {
    std::vector<std::string> tokens;
    std::string word;
    for (std::size_t i = 0; i < text.size(); ++i) {
        char c = text[i];
        bool wordCharacter = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
        if (wordCharacter) {
            word = word + c;
            continue;
        }
        if (!word.empty()) {
            tokens.push_back(word);
            word = "";
        }
        if (c != ' ') {
            tokens.push_back(std::string(1, c));
        }
    }
    if (!word.empty()) {
        tokens.push_back(word);
    }
    return tokens;
}

// how tightly an operator binds: * and / come before + and -
int precedence(const std::string& op) {
    if (op == "*" || op == "/") {
        return 2;
    }
    return 1;
}

bool isOperator(const std::string& token) {
    return token == "+" || token == "-" || token == "*" || token == "/";
}

// Step 2: the parser puts the expression in postfix order with a stack.
// "a + b * 2" becomes "a b 2 * +": operands go straight out, an operator waits on the
// stack until every operator that binds at least as tightly has been written
std::vector<std::string> toPostfix(const std::vector<std::string>& tokens) {
    std::vector<std::string> output;
    std::vector<std::string> waiting;
    for (std::size_t i = 0; i < tokens.size(); ++i) {
        const std::string& token = tokens[i];
        if (isOperator(token)) {
            while (!waiting.empty() && precedence(waiting.back()) >= precedence(token)) {
                output.push_back(waiting.back());
                waiting.pop_back();
            }
            waiting.push_back(token);
        } else {
            output.push_back(token);
        }
    }
    while (!waiting.empty()) {
        output.push_back(waiting.back());
        waiting.pop_back();
    }
    return output;
}

// does the token start with a digit (a constant, not a variable)
bool isNumber(const std::string& token) {
    return !token.empty() && token[0] >= '0' && token[0] <= '9';
}

// Step 3: the code generator walks the postfix with a stack of registers.
// Every operand is loaded into the next free register; every operator combines the top two
// registers into the lower one and frees the upper one
std::vector<std::string> generateCode(const std::vector<std::string>& postfix, const std::string& target) {
    const char* registerNames[4] = {"rbx", "rsi", "rdi", "r8"};
    std::vector<std::string> code;
    std::vector<int> inUse;                              // stack of registers holding values
    for (std::size_t i = 0; i < postfix.size(); ++i) {
        const std::string& token = postfix[i];
        if (!isOperator(token)) {
            int reg = static_cast<int>(inUse.size());
            if (isNumber(token)) {
                code.push_back(std::string("mov ") + registerNames[reg] + ", " + token);
            } else {
                code.push_back(std::string("mov ") + registerNames[reg] + ", [" + token + "]");
            }
            inUse.push_back(reg);
            continue;
        }
        int right = inUse.back();
        inUse.pop_back();
        int left = inUse.back();
        std::string mnemonic = "add";
        if (token == "-") {
            mnemonic = "sub";
        } else if (token == "*") {
            mnemonic = "imul";
        }
        code.push_back(mnemonic + " " + registerNames[left] + ", " + registerNames[right]);
    }
    code.push_back("mov [" + target + "], " + registerNames[0]);
    return code;
}

// the whole compiler: "c = a + b * 2" -> assembly lines
std::vector<std::string> compileAssignment(const std::string& statement) {
    std::vector<std::string> tokens = tokenize(statement);
    std::string target = tokens[0];                      // the variable on the left of '='
    std::vector<std::string> expression(tokens.begin() + 2, tokens.end());
    return generateCode(toPostfix(expression), target);
}
