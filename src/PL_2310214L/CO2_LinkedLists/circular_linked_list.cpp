// A circular linked list of the five instruction phases, where the phase after the last one is the first.

#include <iostream>
#include <string>

struct Node {
    std::string phase;
    Node* next;
};

class PhaseRing {
public:
    PhaseRing() : tail_(nullptr), size_(0) {
    }

    ~PhaseRing() {
        clear();
    }

    // inserts after the tail: the new node points to the head, and becomes the tail
    void insert(const std::string& phase) {
        Node* node = new Node;
        node->phase = phase;
        if (tail_ == nullptr) {
            node->next = node;              // a single node points to itself
            tail_ = node;
        } else {
            node->next = tail_->next;
            tail_->next = node;
            tail_ = node;
        }
        size_ = size_ + 1;
    }

    // the first node of the ring
    Node* head() const {
        if (tail_ == nullptr) {
            return nullptr;
        }
        return tail_->next;
    }

    // walks at most one full lap looking for a phase
    Node* find(const std::string& phase) const {
        Node* node = head();
        for (int step = 0; step < size_; ++step) {
            if (node->phase == phase) {
                return node;
            }
            node = node->next;
        }
        return nullptr;
    }

    // the phase that comes after this one (wraps from the last to the first)
    std::string after(const std::string& phase) const {
        Node* node = find(phase);
        if (node == nullptr) {
            return "";
        }
        return node->next->phase;
    }

    // removes a phase: its predecessor is found by going once round the ring
    bool remove(const std::string& phase) {
        if (tail_ == nullptr) {
            return false;
        }
        Node* previous = tail_;
        Node* node = tail_->next;
        for (int step = 0; step < size_; ++step) {
            if (node->phase == phase) {
                if (size_ == 1) {
                    tail_ = nullptr;
                } else {
                    previous->next = node->next;
                    if (node == tail_) {
                        tail_ = previous;
                    }
                }
                delete node;
                size_ = size_ - 1;
                return true;
            }
            previous = node;
            node = node->next;
        }
        return false;
    }

    // breaks the ring open, then deletes the nodes like a normal list
    void clear() {
        if (tail_ == nullptr) {
            return;
        }
        Node* node = tail_->next;
        tail_->next = nullptr;
        while (node != nullptr) {
            Node* following = node->next;
            delete node;
            node = following;
        }
        tail_ = nullptr;
        size_ = 0;
    }

    int size() const {
        return size_;
    }

private:
    Node* tail_;
    int size_;
};
