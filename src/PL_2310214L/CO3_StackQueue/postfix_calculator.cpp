// A stack and a queue that turn an infix expression into postfix and then calculate its value.

#include <cmath>
#include <iostream>
#include <string>
#include <vector>

const int SIZE = 64;

// a stack of single characters (the operators waiting for their operands)
class CharStack {
public:
    CharStack() : top_(0) {
    }
    void push(char c) {
        items_[top_] = c;
        top_ = top_ + 1;
    }
    char pop() {
        top_ = top_ - 1;
        return items_[top_];
    }
    char peek() const {
        return items_[top_ - 1];
    }
    bool empty() const {
        return top_ == 0;
    }
    std::string text() const {
        std::string result = "[";
        for (int i = 0; i < top_; ++i) {
            result = result + items_[i];
            if (i + 1 < top_) {
                result = result + " ";
            }
        }
        return result + "]";
    }

private:
    char items_[SIZE];
    int top_;
};

// a stack of numbers (the values while the postfix is evaluated)
class NumberStack {
public:
    NumberStack() : top_(0) {
    }
    void push(double value) {
        items_[top_] = value;
        top_ = top_ + 1;
    }
    double pop() {
        top_ = top_ - 1;
        return items_[top_];
    }
    int size() const {
        return top_;
    }

private:
    double items_[SIZE];
    int top_;
};

// a circular queue of strings: 'front_' is the oldest item, items join at the rear
class TokenQueue {
public:
    TokenQueue() : front_(0), count_(0) {
    }
    void enqueue(const std::string& token) {
        int rear = (front_ + count_) % SIZE;        // the rear wraps round the array
        items_[rear] = token;
        count_ = count_ + 1;
    }
    std::string dequeue() {
        std::string token = items_[front_];
        front_ = (front_ + 1) % SIZE;
        count_ = count_ - 1;
        return token;
    }
    bool empty() const {
        return count_ == 0;
    }
    std::string text() const {
        std::string result;
        for (int i = 0; i < count_; ++i) {
            result = result + items_[(front_ + i) % SIZE] + " ";
        }
        return result;
    }

private:
    std::string items_[SIZE];
    int front_;
    int count_;
};

bool isOperator(char c) {
    return c == '+' || c == '-' || c == '*' || c == '/' || c == '^';
}

// + and - bind weakest, * and / next, ^ strongest
int precedence(char op) {
    if (op == '^') {
        return 3;
    }
    if (op == '*' || op == '/') {
        return 2;
    }
    return 1;
}

// 2 ^ 3 ^ 2 means 2 ^ (3 ^ 2); every other operator groups from the left
bool rightAssociative(char op) {
    return op == '^';
}

// cuts "3+4*2" into "3" "+" "4" "*" "2"
std::vector<std::string> tokenize(const std::string& text) {
    std::vector<std::string> tokens;
    std::string number;
    for (std::size_t i = 0; i < text.size(); ++i) {
        char c = text[i];
        bool digit = (c >= '0' && c <= '9') || c == '.';
        if (digit) {
            number = number + c;
            continue;
        }
        if (!number.empty()) {
            tokens.push_back(number);
            number = "";
        }
        if (isOperator(c) || c == '(' || c == ')') {
            tokens.push_back(std::string(1, c));
        }
    }
    if (!number.empty()) {
        tokens.push_back(number);
    }
    return tokens;
}

// step 1: infix -> postfix; prints every step when 'show' is true
bool toPostfix(const std::string& text, TokenQueue& output, std::string& error, bool show) {
    std::vector<std::string> tokens = tokenize(text);
    CharStack operators;
    for (std::size_t i = 0; i < tokens.size(); ++i) {
        std::string token = tokens[i];
        char first = token[0];
        bool number = (first >= '0' && first <= '9') || first == '.';

        if (number) {
            output.enqueue(token);                      // numbers go straight to the output
        } else if (first == '(') {
            operators.push('(');
        } else if (first == ')') {
            while (!operators.empty() && operators.peek() != '(') {
                output.enqueue(std::string(1, operators.pop()));
            }
            if (operators.empty()) {
                error = "missing '(' for a ')'";
                return false;
            }
            operators.pop();                            // throw the '(' away
        } else {
            // operators already on the stack that must be applied first leave first
            while (!operators.empty() && operators.peek() != '(') {
                char top = operators.peek();
                bool popIt = false;
                if (precedence(top) > precedence(first)) {
                    popIt = true;
                }
                if (precedence(top) == precedence(first) && !rightAssociative(first)) {
                    popIt = true;
                }
                if (!popIt) {
                    break;
                }
                output.enqueue(std::string(1, operators.pop()));
            }
            operators.push(first);
        }
        if (show) {
            std::string stackText = operators.text();
            while (stackText.size() < 16) {
                stackText = stackText + " ";
            }
            std::string tokenText = token;
            while (tokenText.size() < 3) {
                tokenText = tokenText + " ";
            }
            std::cout << "    token " << tokenText << "  stack " << stackText << " output queue: " << output.text() << "\n";
        }
    }
    while (!operators.empty()) {
        char op = operators.pop();
        if (op == '(') {
            error = "missing ')' for a '('";
            return false;
        }
        output.enqueue(std::string(1, op));
    }
    return true;
}

// step 2: evaluate the postfix queue with a stack of numbers
bool evaluate(TokenQueue& postfix, double& result, std::string& error) {
    NumberStack values;
    while (!postfix.empty()) {
        std::string token = postfix.dequeue();
        char first = token[0];
        bool number = (first >= '0' && first <= '9') || first == '.';
        if (number) {
            values.push(std::stod(token));
            continue;
        }
        if (values.size() < 2) {
            error = "operator '" + token + "' needs two numbers";
            return false;
        }
        double right = values.pop();                    // the number pushed last is the right operand
        double left = values.pop();
        double answer = 0;
        if (first == '+') {
            answer = left + right;
        } else if (first == '-') {
            answer = left - right;
        } else if (first == '*') {
            answer = left * right;
        } else if (first == '/') {
            if (right == 0) {
                error = "division by zero";
                return false;
            }
            answer = left / right;
        } else {
            answer = std::pow(left, right);
        }
        values.push(answer);
    }
    result = values.pop();
    return true;
}
