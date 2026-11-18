// Stack<T> member functions                                   [PL CO3]
#ifndef VAS_STACK_IMPL
#define VAS_STACK_IMPL
#include "engine/ds/Stack.h"

namespace vas::ds {

// deep copy: a new array with the same elements
template <typename T>
Stack<T>::Stack(const Stack& o) : size_(o.size_), cap_(o.cap_) {
    if (!cap_) {
        return;
    }
    data_ = new T[cap_];
    for (std::size_t i = 0; i < size_; ++i) {
        data_[i] = o.data_[i];
    }
}

// PUSH: put the value on top, growing the array if it is full
template <typename T>
void Stack<T>::push(const T& v) {
    if (size_ == cap_) {
        grow();
    }
    data_[size_++] = v;
}

// POP: remove and return the top value (underflow if empty)
template <typename T>
T Stack<T>::pop() {
    if (empty()) {
        throw std::underflow_error("Stack underflow");
    }
    return std::move(data_[--size_]);
}

// PEEK: look at the top value without removing it
template <typename T>
T& Stack<T>::top() {
    if (empty()) {
        throw std::underflow_error("Stack is empty");
    }
    return data_[size_ - 1];
}

template <typename T>
const T& Stack<T>::top() const {
    if (empty()) {
        throw std::underflow_error("Stack is empty");
    }
    return data_[size_ - 1];
}

template <typename T>
void Stack<T>::swap(Stack& o) noexcept {
    std::swap(data_, o.data_);
    std::swap(size_, o.size_);
    std::swap(cap_, o.cap_);
}

// double the capacity and move the elements across
template <typename T>
void Stack<T>::grow() {
    std::size_t nc = 8;
    if (cap_ > 0) {
        nc = cap_ * 2;
    }
    T* nd = new T[nc];
    for (std::size_t i = 0; i < size_; ++i) {
        nd[i] = std::move(data_[i]);
    }
    delete[] data_;
    data_ = nd;
    cap_ = nc;
}

} // namespace vas::ds
#endif
