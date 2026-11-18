// Two stacks that give undo and redo for a text editor.

#include <iostream>
#include <string>

const int DEPTH = 100;

// a stack of texts stored in an array
class TextStack {
public:
    TextStack() : top_(0) {
    }

    // returns false when the stack is full (the oldest history is then lost)
    bool push(const std::string& text) {
        if (top_ == DEPTH) {
            return false;
        }
        items_[top_] = text;
        top_ = top_ + 1;
        return true;
    }

    std::string pop() {
        top_ = top_ - 1;
        return items_[top_];
    }

    bool empty() const {
        return top_ == 0;
    }

    void clear() {
        top_ = 0;
    }

    int size() const {
        return top_;
    }

private:
    std::string items_[DEPTH];
    int top_;
};

class Editor {
public:
    Editor() {
    }

    // a normal edit: the present becomes the past, and the old future is forgotten
    void edit(const std::string& newText) {
        undoStack_.push(text_);
        redoStack_.clear();
        text_ = newText;
    }

    // Ctrl+Z: the present becomes the future, the most recent past comes back
    bool undo() {
        if (undoStack_.empty()) {
            return false;
        }
        redoStack_.push(text_);
        text_ = undoStack_.pop();
        return true;
    }

    // Ctrl+Y: the present becomes the past again, the most recently undone text comes back
    bool redo() {
        if (redoStack_.empty()) {
            return false;
        }
        undoStack_.push(text_);
        text_ = redoStack_.pop();
        return true;
    }

    void show(const std::string& what) const {
        std::string label = what;
        while (label.size() < 26) {
            label = label + " ";
        }
        std::cout << "  " << label << " text = \"" << text_ << "\"   undo " << undoStack_.size() << "  redo "
                  << redoStack_.size() << "\n";
    }

private:
    std::string text_;
    TextStack undoStack_;
    TextStack redoStack_;
};
