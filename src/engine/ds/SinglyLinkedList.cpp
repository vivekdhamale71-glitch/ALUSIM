// SinglyLinkedList<T> member functions                        [PL CO2]
#ifndef VAS_SLL_IMPL
#define VAS_SLL_IMPL
#include "engine/ds/SinglyLinkedList.h"

namespace vas::ds {

template <typename T>
SinglyLinkedList<T>::SinglyLinkedList(const SinglyLinkedList& o) {
    for (Node* n = o.head_; n; n = n->next) {
        pushBack(n->value);
    }
}

// insert at the end: link the old tail to the new node
template <typename T>
void SinglyLinkedList<T>::pushBack(const T& v) {
    Node* n = new Node(v);
    if (!head_) {
        head_ = tail_ = n;
    } else {
        tail_->next = n;
        tail_ = n;
    }
    ++size_;
}

// delete every node, front to back
template <typename T>
void SinglyLinkedList<T>::clear() {
    while (head_) {
        Node* n = head_->next;
        delete head_;
        head_ = n;
    }
    tail_ = nullptr;
    size_ = 0;
}

template <typename T>
T& SinglyLinkedList<T>::front() {
    if (!head_) {
        throw std::out_of_range("empty list");
    }
    return head_->value;
}

template <typename T>
void SinglyLinkedList<T>::swap(SinglyLinkedList& o) noexcept {
    std::swap(head_, o.head_);
    std::swap(tail_, o.tail_);
    std::swap(size_, o.size_);
}

} // namespace vas::ds
#endif
