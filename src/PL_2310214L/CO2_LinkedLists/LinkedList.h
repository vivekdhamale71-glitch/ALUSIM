// =====================================================================
//  Linked lists                                              [PL CO2]
//  Each list type is used where its shape fits the job:
//   * SinglyLinkedList   - the token stream produced by the lexer
//                          (append-only, walked front to back once)
//   * DoublyLinkedList   - the execution timeline: every node is a CPU
//                          snapshot, so "Step Back" walks prev and
//                          "Step" re-walks next (undo / redo)
//   * CircularLinkedList - the instruction-cycle ring
//                          FETCH -> DECODE -> EXECUTE -> MEMORY -> WB ->
//                          FETCH ... driven by the clock
// =====================================================================
#pragma once
#include <cstddef>
#include <stdexcept>
#include <utility>

namespace vas::ds {

// ------------------------------------------------------------------ singly
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
        Iterator& operator++() { n_ = n_->next; return *this; }
        bool operator!=(const Iterator& o) const { return n_ != o.n_; }
        bool operator==(const Iterator& o) const { return n_ == o.n_; }
    private:
        Node* n_;
    };

    SinglyLinkedList() = default;
    SinglyLinkedList(const SinglyLinkedList& o) { for (Node* n = o.head_; n; n = n->next) pushBack(n->value); }
    SinglyLinkedList(SinglyLinkedList&& o) noexcept { swap(o); }
    SinglyLinkedList& operator=(SinglyLinkedList o) noexcept { swap(o); return *this; }
    ~SinglyLinkedList() { clear(); }

    void pushBack(const T& v) {
        Node* n = new Node(v);
        if (!head_) head_ = tail_ = n;
        else { tail_->next = n; tail_ = n; }
        ++size_;
    }
    void clear() {
        while (head_) { Node* n = head_->next; delete head_; head_ = n; }
        tail_ = nullptr;
        size_ = 0;
    }
    std::size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }
    T& front() { if (!head_) throw std::out_of_range("empty list"); return head_->value; }

    Iterator begin() const { return Iterator(head_); }
    Iterator end() const { return Iterator(nullptr); }

    void swap(SinglyLinkedList& o) noexcept {
        std::swap(head_, o.head_);
        std::swap(tail_, o.tail_);
        std::swap(size_, o.size_);
    }

private:
    Node* head_ = nullptr;
    Node* tail_ = nullptr;
    std::size_t size_ = 0;
};

// ------------------------------------------------------------------ doubly
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

    Node* pushBack(const T& v) {
        Node* n = new Node(v);
        n->prev = tail_;
        if (tail_) tail_->next = n; else head_ = n;
        tail_ = n;
        ++size_;
        return n;
    }
    // remove every node after `keep` (used to drop the "redo" branch when
    // the user steps back and then executes a different path)
    void truncateAfter(Node* keep) {
        Node* n = keep ? keep->next : head_;
        if (keep) keep->next = nullptr; else head_ = nullptr;
        tail_ = keep;
        while (n) { Node* nx = n->next; delete n; --size_; n = nx; }
    }
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

// ---------------------------------------------------------------- circular
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

    void insert(const T& v) {
        Node* n = new Node(v);
        if (!tail_) { n->next = n; tail_ = n; }
        else { n->next = tail_->next; tail_->next = n; tail_ = n; }
        ++size_;
    }
    Node* head() const { return tail_ ? tail_->next : nullptr; }
    std::size_t size() const { return size_; }
    void clear() {
        if (!tail_) return;
        Node* n = tail_->next;
        tail_->next = nullptr;
        while (n) { Node* nx = n->next; delete n; n = nx; }
        tail_ = nullptr;
        size_ = 0;
    }

private:
    Node* tail_ = nullptr;
    std::size_t size_ = 0;
};

} // namespace vas::ds
