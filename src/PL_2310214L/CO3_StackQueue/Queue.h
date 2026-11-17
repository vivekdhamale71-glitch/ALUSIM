// =====================================================================
//  CircularQueue<T>  -  ring buffer queue                    [PL CO3]
//  Used by: shunting-yard output queue (Compiler), the rolling
//  control-signal history behind the waveform view (overwrite mode),
//  and the token stream fed to the parser.
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
    explicit CircularQueue(std::size_t capacity = 16, bool overwrite = false)
        : cap_(capacity ? capacity : 1), overwrite_(overwrite) {
        data_ = new T[cap_];
    }
    CircularQueue(const CircularQueue& o)
        : cap_(o.cap_), head_(o.head_), count_(o.count_), overwrite_(o.overwrite_) {
        data_ = new T[cap_];
        for (std::size_t i = 0; i < cap_; ++i) data_[i] = o.data_[i];
    }
    CircularQueue(CircularQueue&& o) noexcept { swap(o); }
    CircularQueue& operator=(CircularQueue o) noexcept { swap(o); return *this; }
    ~CircularQueue() { delete[] data_; }

    void enqueue(const T& v) {
        if (full()) {
            if (overwrite_) {
                head_ = (head_ + 1) % cap_;
                --count_;
            } else {
                grow();
            }
        }
        data_[(head_ + count_) % cap_] = v;
        ++count_;
    }
    T dequeue() {
        if (empty()) throw std::underflow_error("Queue underflow");
        T v = std::move(data_[head_]);
        head_ = (head_ + 1) % cap_;
        --count_;
        return v;
    }
    const T& front() const {
        if (empty()) throw std::underflow_error("Queue is empty");
        return data_[head_];
    }
    // i = 0 is the front (oldest)
    const T& at(std::size_t i) const { return data_[(head_ + i) % cap_]; }
    const T& back() const { return at(count_ - 1); }

    bool empty() const { return count_ == 0; }
    bool full() const { return count_ == cap_; }
    std::size_t size() const { return count_; }
    std::size_t capacity() const { return cap_; }
    void clear() { head_ = count_ = 0; }

    void swap(CircularQueue& o) noexcept {
        std::swap(data_, o.data_);
        std::swap(cap_, o.cap_);
        std::swap(head_, o.head_);
        std::swap(count_, o.count_);
        std::swap(overwrite_, o.overwrite_);
    }

private:
    void grow() {
        std::size_t nc = cap_ * 2;
        T* nd = new T[nc];
        for (std::size_t i = 0; i < count_; ++i) nd[i] = std::move(data_[(head_ + i) % cap_]);
        delete[] data_;
        data_ = nd;
        cap_ = nc;
        head_ = 0;
    }

    T* data_ = nullptr;
    std::size_t cap_ = 1;
    std::size_t head_ = 0;
    std::size_t count_ = 0;
    bool overwrite_ = false;
};

} // namespace vas::ds
