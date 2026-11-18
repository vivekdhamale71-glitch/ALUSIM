// CircularQueue<T> member functions                           [PL CO3]
#ifndef VAS_QUEUE_IMPL
#define VAS_QUEUE_IMPL
#include "engine/ds/Queue.h"

namespace vas::ds {

template <typename T>
CircularQueue<T>::CircularQueue(std::size_t capacity, bool overwrite) : overwrite_(overwrite) {
    cap_ = capacity;
    if (cap_ == 0) {
        cap_ = 1;
    }
    data_ = new T[cap_];
}

template <typename T>
CircularQueue<T>::CircularQueue(const CircularQueue& o)
    : cap_(o.cap_), head_(o.head_), count_(o.count_), overwrite_(o.overwrite_) {
    data_ = new T[cap_];
    for (std::size_t i = 0; i < cap_; ++i) {
        data_[i] = o.data_[i];
    }
}

// ENQUEUE at the rear. When full: drop the oldest (overwrite) or grow.
template <typename T>
void CircularQueue<T>::enqueue(const T& v) {
    if (full()) {
        if (overwrite_) {
            head_ = (head_ + 1) % cap_;
            --count_;
        } else {
            grow();
        }
    }
    data_[(head_ + count_) % cap_] = v;       // rear wraps around
    ++count_;
}

// DEQUEUE from the front
template <typename T>
T CircularQueue<T>::dequeue() {
    if (empty()) {
        throw std::underflow_error("Queue underflow");
    }
    T v = std::move(data_[head_]);
    head_ = (head_ + 1) % cap_;
    --count_;
    return v;
}

template <typename T>
const T& CircularQueue<T>::front() const {
    if (empty()) {
        throw std::underflow_error("Queue is empty");
    }
    return data_[head_];
}

template <typename T>
void CircularQueue<T>::swap(CircularQueue& o) noexcept {
    std::swap(data_, o.data_);
    std::swap(cap_, o.cap_);
    std::swap(head_, o.head_);
    std::swap(count_, o.count_);
    std::swap(overwrite_, o.overwrite_);
}

// double the capacity and unwrap the elements to start at index 0
template <typename T>
void CircularQueue<T>::grow() {
    std::size_t nc = cap_ * 2;
    T* nd = new T[nc];
    for (std::size_t i = 0; i < count_; ++i) {
        nd[i] = std::move(data_[(head_ + i) % cap_]);
    }
    delete[] data_;
    data_ = nd;
    cap_ = nc;
    head_ = 0;
}

} // namespace vas::ds
#endif
