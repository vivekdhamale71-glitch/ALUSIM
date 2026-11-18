// A stack that matches brackets and reports the line and column of the first mistake.

#include <iostream>
#include <string>

const int STACK_SIZE = 200;

// an opening bracket that is still waiting for its partner
struct OpenBracket {
    char symbol;
    int line;
    int column;
};

// a stack stored in an array: 'top_' is the number of items, the last one is on top
class BracketStack {
public:
    BracketStack() : top_(0) {
    }

    // puts an item on top: O(1)
    bool push(const OpenBracket& item) {
        if (top_ == STACK_SIZE) {
            return false;                   // stack overflow
        }
        items_[top_] = item;
        top_ = top_ + 1;
        return true;
    }

    // takes the top item off: O(1)
    OpenBracket pop() {
        top_ = top_ - 1;
        return items_[top_];
    }

    // looks at the top item without removing it
    const OpenBracket& peek() const {
        return items_[top_ - 1];
    }

    bool empty() const {
        return top_ == 0;
    }

private:
    OpenBracket items_[STACK_SIZE];
    int top_;
};

bool isOpening(char c) {
    return c == '(' || c == '[' || c == '{';
}

bool isClosing(char c) {
    return c == ')' || c == ']' || c == '}';
}

// the opening bracket that a closing bracket belongs to
char partnerOf(char closing) {
    if (closing == ')') {
        return '(';
    }
    if (closing == ']') {
        return '[';
    }
    return '{';
}

std::string where(int line, int column) {
    return "line " + std::to_string(line) + ", column " + std::to_string(column);
}

// walks through the code once; returns "" if the brackets match, else a message
std::string checkBrackets(const std::string& code) {
    BracketStack waiting;
    int line = 1;
    int column = 0;
    bool inLineComment = false;
    bool inString = false;

    for (std::size_t i = 0; i < code.size(); ++i) {
        char c = code[i];
        column = column + 1;
        if (c == '\n') {
            line = line + 1;
            column = 0;
            inLineComment = false;
            continue;
        }
        if (inLineComment) {
            continue;                       // brackets in comments do not count
        }
        if (inString) {
            if (c == '"') {
                inString = false;
            }
            continue;                       // brackets in strings do not count
        }
        if (c == '"') {
            inString = true;
        } else if (c == '/' && i + 1 < code.size() && code[i + 1] == '/') {
            inLineComment = true;
        } else if (isOpening(c)) {
            OpenBracket bracket;
            bracket.symbol = c;
            bracket.line = line;
            bracket.column = column;
            waiting.push(bracket);          // wait for the partner
        } else if (isClosing(c)) {
            if (waiting.empty()) {
                return std::string("'") + c + "' at " + where(line, column) + " has no opening bracket";
            }
            OpenBracket top = waiting.pop();            // the bracket opened last
            if (top.symbol != partnerOf(c)) {
                return std::string("'") + c + "' at " + where(line, column) + " does not match '" + top.symbol +
                       "' opened at " + where(top.line, top.column);
            }
        }
    }
    if (!waiting.empty()) {
        OpenBracket top = waiting.peek();
        return std::string("'") + top.symbol + "' opened at " + where(top.line, top.column) + " is never closed";
    }
    return "";
}
