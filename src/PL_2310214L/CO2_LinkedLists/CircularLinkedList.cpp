// CircularLinkedList<T> member functions                      [PL CO2]
#ifndef VAS_CLL_IMPL
#define VAS_CLL_IMPL
#include "PL_2310214L/CO2_LinkedLists/CircularLinkedList.h"

namespace vas::ds {

// new node goes between tail and head, then becomes the tail
template <typename T>
void CircularLinkedList<T>::insert(const T& v) {
    Node* n = new Node(v);
    if (!tail_) {
        n->next = n;                    // a single node points to itself
        tail_ = n;
    } else {
        n->next = tail_->next;
        tail_->next = n;
        tail_ = n;
    }
    ++size_;
}

// walk at most one full lap; nullptr if the value is not in the ring
template <typename T>
typename CircularLinkedList<T>::Node* CircularLinkedList<T>::find(const T& v) const {
    Node* n = head();
    for (std::size_t i = 0; i < size_; ++i, n = n->next)
        if (n->value == v) return n;
    return nullptr;
}

// break the ring, then delete it like a singly linked list
template <typename T>
void CircularLinkedList<T>::clear() {
    if (!tail_) return;
    Node* n = tail_->next;
    tail_->next = nullptr;
    while (n) {
        Node* nx = n->next;
        delete n;
        n = nx;
    }
    tail_ = nullptr;
    size_ = 0;
}

} // namespace vas::ds
#endif
