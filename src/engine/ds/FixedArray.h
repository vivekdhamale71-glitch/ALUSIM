// =====================================================================
//  FixedArray<T, N>  -  fixed-size array with bounds checking  [PL CO1]
//  Used as the CPU register file (16 x Register64): registers are
//  addressed by index (RAX = 0 ... R15 = 15), so an array is the
//  natural structure - O(1) access, contiguous storage, fixed size.
//  at() throws std::out_of_range on a bad index; operator[] is the
//  fast path used inside the CPU.
//  Member functions are defined in FixedArray.cpp (included at the end,
//  because a template must be visible where it is used).
// =====================================================================
#pragma once
#include <cstddef>
#include <stdexcept>
#include <string>

namespace vas::ds {

template <typename T, std::size_t N>
class FixedArray {
public:
    FixedArray() = default;
    explicit FixedArray(const T& fill);

    T& operator[](std::size_t i) { return data_[i]; }
    const T& operator[](std::size_t i) const { return data_[i]; }
    T& at(std::size_t i);
    const T& at(std::size_t i) const;

    void fill(const T& v);
    static constexpr std::size_t size() { return N; }

    T* begin() { return data_; }
    T* end() { return data_ + N; }
    const T* begin() const { return data_; }
    const T* end() const { return data_ + N; }

private:
    T data_[N]{};
};

} // namespace vas::ds

#include "engine/ds/FixedArray.cpp"
