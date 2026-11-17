// =====================================================================
//  Stack<T>  -  dynamic-array stack (LIFO)                  [PL CO3]
//  Used by: shunting-yard operator stack (Compiler), register
//  allocation in code generation, CALL/RET return-address view.
//  push / pop / top are O(1); the array doubles when full (amortised
//  O(1)). Rule of five + copy-and-swap                       [PSOOP]
//  Member functions are defined in Stack.cpp (included at the end).
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
    Stack(const Stack& o);
    Stack(Stack&& o) noexcept { swap(o); }
    Stack& operator=(Stack o) noexcept { swap(o); return *this; }
    ~Stack() { delete[] data_; }

    void push(const T& v);
    T pop();
    T& top();
    const T& top() const;
    bool empty() const { return size_ == 0; }
    std::size_t size() const { return size_; }
    void clear() { size_ = 0; }

    // index 0 = bottom of stack (used only for drawing the stack)
    const T& operator[](std::size_t i) const { return data_[i]; }
    void swap(Stack& o) noexcept;

private:
    void grow();

    T* data_ = nullptr;
    std::size_t size_ = 0;
    std::size_t cap_ = 0;
};

} // namespace vas::ds

#include "PL_2310214L/CO3_StackQueue/Stack.cpp"
