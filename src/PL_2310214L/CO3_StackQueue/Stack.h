// =====================================================================
//  Stack<T>  -  dynamic-array stack                         [PL CO3]
//  Used by: shunting-yard operator stack (Compiler), register
//  allocation in code generation, CALL/RET return-address view.
//  Rule of five + copy-and-swap                              [PSOOP]
// =====================================================================
#pragma once
#include <cstddef>
#include <stdexcept>
#include <utility>

namespace vas::ds {

template <typename T>
class Stack {
public:
    Stack() = default;
    Stack(const Stack& o) { copyFrom(o); }
    Stack(Stack&& o) noexcept { swap(o); }
    Stack& operator=(Stack o) noexcept { swap(o); return *this; }
    ~Stack() { delete[] data_; }

    void push(const T& v) {
        if (size_ == cap_) grow();
        data_[size_++] = v;
    }
    T pop() {
        if (empty()) throw std::underflow_error("Stack underflow");
        return std::move(data_[--size_]);
    }
    T& top() {
        if (empty()) throw std::underflow_error("Stack is empty");
        return data_[size_ - 1];
    }
    const T& top() const {
        if (empty()) throw std::underflow_error("Stack is empty");
        return data_[size_ - 1];
    }
    bool empty() const { return size_ == 0; }
    std::size_t size() const { return size_; }
    void clear() { size_ = 0; }

    // index 0 = bottom of stack (used only for drawing the stack)
    const T& operator[](std::size_t i) const { return data_[i]; }

    void swap(Stack& o) noexcept {
        std::swap(data_, o.data_);
        std::swap(size_, o.size_);
        std::swap(cap_, o.cap_);
    }

private:
    void grow() {
        std::size_t nc = cap_ ? cap_ * 2 : 8;
        T* nd = new T[nc];
        for (std::size_t i = 0; i < size_; ++i) nd[i] = std::move(data_[i]);
        delete[] data_;
        data_ = nd;
        cap_ = nc;
    }
    void copyFrom(const Stack& o) {
        if (!o.cap_) return;
        data_ = new T[o.cap_];
        cap_ = o.cap_;
        size_ = o.size_;
        for (std::size_t i = 0; i < size_; ++i) data_[i] = o.data_[i];
    }

    T* data_ = nullptr;
    std::size_t size_ = 0;
    std::size_t cap_ = 0;
};

} // namespace vas::ds
