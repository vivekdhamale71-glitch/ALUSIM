// =====================================================================
//  DoublyLinkedList<T>                                       [PL CO2]
//  The execution timeline: every node holds the CPU snapshot taken
//  before an instruction. Step Back walks to tail->prev and restores it;
//  truncateAfter() drops the nodes after a point.
//  Member functions are in DoublyLinkedList.cpp.
// =====================================================================
#pragma once
#include <cstddef>

namespace vas::ds {

template <typename T>
class DoublyLinkedList {
public:
    struct Node {
        T value;
        Node* prev = nullptr;
        Node* next = nullptr;
        explicit Node(const T& v) : value(v) {}
    };

    DoublyLinkedList() = default;
    DoublyLinkedList(const DoublyLinkedList&) = delete;            // owns snapshots
    DoublyLinkedList& operator=(const DoublyLinkedList&) = delete;
    ~DoublyLinkedList() { clear(); }

    Node* pushBack(const T& v);
    void truncateAfter(Node* keep);
    void clear() { truncateAfter(nullptr); }

    Node* head() const { return head_; }
    Node* tail() const { return tail_; }
    std::size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }

private:
    Node* head_ = nullptr;
    Node* tail_ = nullptr;
    std::size_t size_ = 0;
};

} // namespace vas::ds

#include "engine/ds/DoublyLinkedList.cpp"
