// binarySearch<T>                                             [PL CO4]
#ifndef VAS_TABLESEARCH_IMPL
#define VAS_TABLESEARCH_IMPL
#include "engine/ds/TableSearch.h"

namespace vas::ds {

template <typename T, typename K, typename Cmp>
int binarySearch(const std::vector<T>& a, const K& key, Cmp cmp, std::vector<int>* probes) {
    int lo = 0, hi = int(a.size()) - 1;
    while (lo <= hi) {
        int mid = lo + (hi - lo) / 2;
        if (probes) {
            probes->push_back(mid);
        }
        int c = cmp(a[mid], key);
        if (c == 0) {
            return mid;
        }
        if (c < 0) {
            lo = mid + 1;
        }        // key is in the right half
        else {
            hi = mid - 1;
        }              // key is in the left half
    }
    return -1;
}

} // namespace vas::ds
#endif
