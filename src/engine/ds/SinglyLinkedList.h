// =====================================================================
//  SinglyLinkedList<T>                                       [PL CO2]
//  The lexer's token stream: tokens are appended at the tail (O(1) with
//  a tail pointer) and the parser walks them once, front to back, with
//  an iterator.  Member functions are in SinglyLinkedList.cpp.
// =====================================================================
#pragma once
#include <cstddef>
#include <stdexcept>
#include <utility>

namespace vas::ds {

template <typename T>
class SinglyLinkedList {
    struct Node {
        T value;
        Node* next = nullptr;
        explicit Node(const T& v) : value(v) {}
    };

public:
    class Iterator {
    public:
        explicit Iterator(Node* n) : n_(n) {}
        T& operator*() const { return n_->value; }
        T* operator->() const { return &n_->value; }
        Iterator& operator++() {
            n_ = n_->next;
            return *this;
        }
        bool operator!=(const Iterator& o) const { return n_ != o.n_; }
        bool operator==(const Iterator& o) const { return n_ == o.n_; }
    private:
        Node* n_;
    };

    SinglyLinkedList() = default;
    SinglyLinkedList(const SinglyLinkedList& o);
    SinglyLinkedList(SinglyLinkedList&& o) noexcept { swap(o); }
    SinglyLinkedList& operator=(SinglyLinkedList o) noexcept {
        swap(o);
        return *this;
    }
    ~SinglyLinkedList() { clear(); }

    void pushBack(const T& v);
    void clear();
    T& front();
    std::size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }

    Iterator begin() const { return Iterator(head_); }
    Iterator end() const { return Iterator(nullptr); }
    void swap(SinglyLinkedList& o) noexcept;

private:
    Node* head_ = nullptr;
    Node* tail_ = nullptr;
    std::size_t size_ = 0;
};

} // namespace vas::ds

#include "engine/ds/SinglyLinkedList.cpp"
