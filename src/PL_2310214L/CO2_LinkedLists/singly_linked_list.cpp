// A singly linked list that stores the tokens of a statement: add at the ends, insert, find, remove and reverse.

#include <iostream>
#include <string>

struct Node {
    std::string text;
    Node* next;
};

class TokenList {
public:
    TokenList() : head_(nullptr), tail_(nullptr), size_(0) {
    }

    ~TokenList() {
        clear();
    }

    // adds at the end: the old tail points to the new node, which becomes the tail
    void pushBack(const std::string& text) {
        Node* node = new Node;
        node->text = text;
        node->next = nullptr;
        if (head_ == nullptr) {
            head_ = node;
            tail_ = node;
        } else {
            tail_->next = node;
            tail_ = node;
        }
        size_ = size_ + 1;
    }

    // adds at the front: the new node points to the old head
    void pushFront(const std::string& text) {
        Node* node = new Node;
        node->text = text;
        node->next = head_;
        head_ = node;
        if (tail_ == nullptr) {
            tail_ = node;
        }
        size_ = size_ + 1;
    }

    // inserts a new node right after 'position' (a pointer to an existing node)
    void insertAfter(Node* position, const std::string& text) {
        if (position == nullptr) {
            return;
        }
        Node* node = new Node;
        node->text = text;
        node->next = position->next;
        position->next = node;
        if (position == tail_) {
            tail_ = node;
        }
        size_ = size_ + 1;
    }

    // finds the first node holding this text (walks the chain: O(n))
    Node* find(const std::string& text) const {
        for (Node* node = head_; node != nullptr; node = node->next) {
            if (node->text == text) {
                return node;
            }
        }
        return nullptr;
    }

    // removes the first node holding this text: the node before it skips over it
    bool remove(const std::string& text) {
        Node* previous = nullptr;
        for (Node* node = head_; node != nullptr; node = node->next) {
            if (node->text == text) {
                if (previous == nullptr) {
                    head_ = node->next;
                } else {
                    previous->next = node->next;
                }
                if (node == tail_) {
                    tail_ = previous;
                }
                delete node;
                size_ = size_ - 1;
                return true;
            }
            previous = node;
        }
        return false;
    }

    // reverses the chain by turning every link around
    void reverse() {
        Node* previous = nullptr;
        Node* node = head_;
        tail_ = head_;
        while (node != nullptr) {
            Node* following = node->next;
            node->next = previous;
            previous = node;
            node = following;
        }
        head_ = previous;
    }

    // deletes every node, front to back
    void clear() {
        while (head_ != nullptr) {
            Node* following = head_->next;
            delete head_;
            head_ = following;
        }
        tail_ = nullptr;
        size_ = 0;
    }

    int size() const {
        return size_;
    }

    Node* head() const {
        return head_;
    }

    std::string text() const {
        std::string result;
        for (Node* node = head_; node != nullptr; node = node->next) {
            result = result + "[" + node->text + "]";
            if (node->next != nullptr) {
                result = result + " -> ";
            }
        }
        return result;
    }

private:
    Node* head_;
    Node* tail_;
    int size_;
};

// the lexer's job: cut a statement into identifiers, numbers and operators
void tokenize(const std::string& statement, TokenList& tokens) {
    std::string word;
    for (std::size_t i = 0; i < statement.size(); ++i) {
        char c = statement[i];
        bool wordCharacter = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
        if (wordCharacter) {
            word = word + c;
            continue;
        }
        if (!word.empty()) {
            tokens.pushBack(word);
            word = "";
        }
        if (c != ' ') {
            tokens.pushBack(std::string(1, c));
        }
    }
    if (!word.empty()) {
        tokens.pushBack(word);
    }
}
