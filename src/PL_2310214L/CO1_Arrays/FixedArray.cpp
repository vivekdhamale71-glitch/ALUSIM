// FixedArray<T, N> member functions                           [PL CO1]
#ifndef VAS_FIXEDARRAY_IMPL
#define VAS_FIXEDARRAY_IMPL
#include "PL_2310214L/CO1_Arrays/FixedArray.h"

namespace vas::ds {

template <typename T, std::size_t N>
FixedArray<T, N>::FixedArray(const T& v) {
    fill(v);
}

// checked access: index must be 0 .. N-1
template <typename T, std::size_t N>
T& FixedArray<T, N>::at(std::size_t i) {
    if (i >= N) throw std::out_of_range("FixedArray index " + std::to_string(i) + " out of range");
    return data_[i];
}

template <typename T, std::size_t N>
const T& FixedArray<T, N>::at(std::size_t i) const {
    if (i >= N) throw std::out_of_range("FixedArray index " + std::to_string(i) + " out of range");
    return data_[i];
}

// traverse every element: O(N)
template <typename T, std::size_t N>
void FixedArray<T, N>::fill(const T& v) {
    for (std::size_t i = 0; i < N; ++i) data_[i] = v;
}

} // namespace vas::ds
#endif
