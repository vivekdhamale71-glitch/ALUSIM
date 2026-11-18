// A doubly linked list that stores an undo history: step back, remove from the middle and walk in both directions.

#include <iostream>
#include <string>

struct Node {
    std::string snapshot;
    Node* previous;
    Node* next;
};

class History {
public:
    History() : head_(nullptr), tail_(nullptr), size_(0) {
    }

    ~History() {
        truncateAfter(nullptr);
    }

    // records a new state at the end: new->previous = tail, tail->next = new
    void push(const std::string& snapshot) {
        Node* node = new Node;
        node->snapshot = snapshot;
        node->previous = tail_;
        node->next = nullptr;
        if (tail_ != nullptr) {
            tail_->next = node;
        } else {
            head_ = node;
        }
        tail_ = node;
        size_ = size_ + 1;
    }

    // Step Back: removes the newest state and returns the one before it
    // (returns false when there is nothing older)
    bool stepBack(std::string& restored) {
        if (tail_ == nullptr || tail_->previous == nullptr) {
            return false;
        }
        Node* newest = tail_;
        tail_ = tail_->previous;            // the node before the tail becomes the tail
        tail_->next = nullptr;
        restored = tail_->snapshot;
        delete newest;
        size_ = size_ - 1;
        return true;
    }

    // deletes every node after 'keep' (keep = nullptr deletes the whole list)
    void truncateAfter(Node* keep) {
        Node* node = head_;
        if (keep != nullptr) {
            node = keep->next;
            keep->next = nullptr;
        } else {
            head_ = nullptr;
        }
        tail_ = keep;
        while (node != nullptr) {
            Node* following = node->next;
            delete node;
            size_ = size_ - 1;
            node = following;
        }
    }

    // removes one node from the middle in O(1): its two neighbours are joined
    void removeNode(Node* node) {
        if (node == nullptr) {
            return;
        }
        if (node->previous != nullptr) {
            node->previous->next = node->next;
        } else {
            head_ = node->next;
        }
        if (node->next != nullptr) {
            node->next->previous = node->previous;
        } else {
            tail_ = node->previous;
        }
        delete node;
        size_ = size_ - 1;
    }

    // prints oldest -> newest, then newest -> oldest (both directions work)
    void printBothWays() const {
        std::cout << "  forward : ";
        for (Node* node = head_; node != nullptr; node = node->next) {
            std::cout << "[" << node->snapshot << "] ";
        }
        std::cout << "\n  backward: ";
        for (Node* node = tail_; node != nullptr; node = node->previous) {
            std::cout << "[" << node->snapshot << "] ";
        }
        std::cout << "\n";
    }

    Node* head() const {
        return head_;
    }

    int size() const {
        return size_;
    }

private:
    Node* head_;
    Node* tail_;
    int size_;
};
