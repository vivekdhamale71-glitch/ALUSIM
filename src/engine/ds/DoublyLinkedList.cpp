// DoublyLinkedList<T> member functions                        [PL CO2]
#ifndef VAS_DLL_IMPL
#define VAS_DLL_IMPL
#include "engine/ds/DoublyLinkedList.h"

namespace vas::ds {

// insert at the end: new->prev = tail, tail->next = new
template <typename T>
typename DoublyLinkedList<T>::Node* DoublyLinkedList<T>::pushBack(const T& v) {
    Node* n = new Node(v);
    n->prev = tail_;
    if (tail_) {
        tail_->next = n;
    } else {
        head_ = n;
    }
    tail_ = n;
    ++size_;
    return n;
}

// delete every node after `keep` (keep = nullptr deletes the whole list);
// Step Back uses it to remove the instruction that was undone
template <typename T>
void DoublyLinkedList<T>::truncateAfter(Node* keep) {
    Node* n = head_;
    if (keep != nullptr) {
        n = keep->next;
    }
    if (keep) {
        keep->next = nullptr;
    } else {
        head_ = nullptr;
    }
    tail_ = keep;
    while (n) {
        Node* nx = n->next;
        delete n;
        --size_;
        n = nx;
    }
}

} // namespace vas::ds
#endif
