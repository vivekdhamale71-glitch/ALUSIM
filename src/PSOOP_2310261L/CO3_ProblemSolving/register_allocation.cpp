// Counts how many registers an expression needs and hands registers out and back with a stack of free registers.

#include <string>
#include <vector>

// a node of an expression tree: a leaf (variable or number) or an operator with two children
struct TreeNode {
    std::string text;
    TreeNode* left;
    TreeNode* right;
};

// a leaf has no children
static bool isLeaf(const TreeNode* node) {
    return node->left == nullptr && node->right == nullptr;
}

// the fewest registers that can evaluate the expression (the Sethi-Ullman number):
//   a leaf needs 1; an operator needs the larger need of its two sides,
//   and one more when both sides need the same (one side's value must wait while the other is worked out)
int registersNeeded(const TreeNode* node) {
    if (isLeaf(node)) {
        return 1;
    }
    int left = registersNeeded(node->left);
    int right = registersNeeded(node->right);
    if (left == right) {
        return left + 1;
    }
    if (left > right) {
        return left;
    }
    return right;
}

// the registers that are free: a stack, so the register freed last is reused first
class FreeRegisters {
public:
    // all registers are free at the start; the lowest number is on top
    explicit FreeRegisters(int count) {
        for (int number = count - 1; number >= 0; --number) {
            free_.push_back(number);
        }
    }

    bool empty() const {
        return free_.empty();
    }

    // takes the register on top of the stack; returns -1 when none is left
    int take() {
        if (free_.empty()) {
            return -1;
        }
        int number = free_.back();
        free_.pop_back();
        return number;
    }

    // gives a register back so that it can be used again
    void give(int number) {
        free_.push_back(number);
    }

private:
    std::vector<int> free_;
};

// generates code for the tree: the result register is returned, or -1 if the registers ran out.
// The side that needs more registers is worked out first, while every register is free; its value is
// held while the other side is worked out. Then both are combined into the left register and the
// right register is given back. With this order the tree never uses more than registersNeeded() registers.
int generate(const TreeNode* node, FreeRegisters& registers, const std::string names[], std::vector<std::string>& code) {
    if (isLeaf(node)) {
        int reg = registers.take();
        if (reg < 0) {
            return -1;
        }
        code.push_back("mov " + names[reg] + ", " + node->text);
        return reg;
    }
    int left = -1;
    int right = -1;
    if (registersNeeded(node->right) > registersNeeded(node->left)) {
        right = generate(node->right, registers, names, code);
        if (right < 0) {
            return -1;
        }
        left = generate(node->left, registers, names, code);
    } else {
        left = generate(node->left, registers, names, code);
        if (left < 0) {
            return -1;
        }
        right = generate(node->right, registers, names, code);
    }
    if (left < 0 || right < 0) {
        return -1;
    }
    std::string mnemonic = "add";
    if (node->text == "-") {
        mnemonic = "sub";
    } else if (node->text == "*") {
        mnemonic = "imul";
    }
    code.push_back(mnemonic + " " + names[left] + ", " + names[right]);
    registers.give(right);
    return left;
}

// makes one node of a tree; the caller owns it and frees the whole tree with freeTree
TreeNode* makeNode(const std::string& text, TreeNode* left, TreeNode* right) {
    TreeNode* node = new TreeNode();
    node->text = text;
    node->left = left;
    node->right = right;
    return node;
}

// deletes a whole tree
void freeTree(TreeNode* node) {
    if (node == nullptr) {
        return;
    }
    freeTree(node->left);
    freeTree(node->right);
    delete node;
}
