// =====================================================================
//  CircularQueue<T>  -  ring buffer queue (FIFO)             [PL CO3]
//  Used by: shunting-yard output queue (Compiler) and the rolling
//  control-signal history behind the waveform view (overwrite mode).
//  front = head_, rear = (head_ + count_) % capacity: both ends move
//  forward and wrap around, so enqueue / dequeue are O(1).
//  Member functions are defined in Queue.cpp (included at the end).
// =====================================================================
#pragma once
#include <cstddef>
#include <stdexcept>
#include <utility>

namespace vas::ds {

template <typename T>
class CircularQueue {
public:
    // overwrite = true  -> when full, the oldest element is dropped
    //                      (a rolling history buffer, e.g. waveforms)
    explicit CircularQueue(std::size_t capacity = 16, bool overwrite = false);
    CircularQueue(const CircularQueue& o);
    CircularQueue(CircularQueue&& o) noexcept { swap(o); }
    CircularQueue& operator=(CircularQueue o) noexcept { swap(o); return *this; }
    ~CircularQueue() { delete[] data_; }

    void enqueue(const T& v);
    T dequeue();
    const T& front() const;
    // i = 0 is the front (oldest)
    const T& at(std::size_t i) const { return data_[(head_ + i) % cap_]; }
    const T& back() const { return at(count_ - 1); }

    bool empty() const { return count_ == 0; }
    bool full() const { return count_ == cap_; }
    std::size_t size() const { return count_; }
    std::size_t capacity() const { return cap_; }
    void clear() { head_ = count_ = 0; }
    void swap(CircularQueue& o) noexcept;

private:
    void grow();

    T* data_ = nullptr;
    std::size_t cap_ = 1;
    std::size_t head_ = 0;
    std::size_t count_ = 0;
    bool overwrite_ = false;
};

} // namespace vas::ds

#include "PL_2310214L/CO3_StackQueue/Queue.cpp"
