// quickSort<T>                                                [PL CO4]
#ifndef VAS_TABLESORT_IMPL
#define VAS_TABLESORT_IMPL
#include <utility>

#include "PL_2310214L/CO4_SearchingSorting/TableSort.h"

namespace vas::ds {

// Hoare partition around the middle element; recurse into the smaller
// side and loop on the larger one, so the stack depth stays O(log n)
template <typename T, typename Less>
void quickSort(std::vector<T>& a, Less less, int lo, int hi) {
    if (hi == -2) hi = int(a.size()) - 1;
    while (lo < hi) {
        T pivot = a[(lo + hi) / 2];
        int i = lo, j = hi;
        while (i <= j) {
            while (less(a[i], pivot)) ++i;
            while (less(pivot, a[j])) --j;
            if (i <= j) { std::swap(a[i], a[j]); ++i; --j; }
        }
        if (j - lo < hi - i) { quickSort(a, less, lo, j); lo = i; }
        else { quickSort(a, less, i, hi); hi = j; }
    }
}

} // namespace vas::ds
#endif
