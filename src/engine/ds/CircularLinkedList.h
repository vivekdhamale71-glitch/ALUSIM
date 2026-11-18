// =====================================================================
//  CircularLinkedList<T>                                     [PL CO2]
//  The instruction-cycle ring FETCH -> DECODE -> EXECUTE -> MEMORY ->
//  WB -> FETCH. The last node points back to the first, so "the phase
//  after X" is simply find(X)->next, with no end-of-list case.
//  Only a tail pointer is kept: head = tail->next.
//  Member functions are in CircularLinkedList.cpp.
// =====================================================================
#pragma once
#include <cstddef>

namespace vas::ds {

template <typename T>
class CircularLinkedList {
public:
    struct Node {
        T value;
        Node* next = nullptr;
        explicit Node(const T& v) : value(v) {}
    };

    CircularLinkedList() = default;
    CircularLinkedList(const CircularLinkedList&) = delete;
    CircularLinkedList& operator=(const CircularLinkedList&) = delete;
    ~CircularLinkedList() { clear(); }

    void insert(const T& v);            // insert after the tail (at the end)
    Node* find(const T& v) const;       // one lap around the ring
    void clear();
    Node* head() const {
        if (tail_ == nullptr) {
            return nullptr;
        }
        return tail_->next;
    }
    std::size_t size() const { return size_; }

private:
    Node* tail_ = nullptr;
    std::size_t size_ = 0;
};

} // namespace vas::ds

#include "engine/ds/CircularLinkedList.cpp"
