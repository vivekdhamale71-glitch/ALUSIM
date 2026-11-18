#include "engine/ds/Searching.h"

#include <algorithm>

namespace vas::ds {

// compare every element in order: O(n), works on unsorted data
int LinearSearch::search(const std::vector<int>& a, int t, std::vector<SearchStep>& s) const {
    for (int i = 0; i < int(a.size()); ++i) {
        bool f = a[i] == t;
        s.push_back({i, 0, int(a.size()) - 1, f});
        if (f) {
            return i;
        }
    }
    return -1;
}

// halve the range [lo, hi] each step: O(log n), data must be sorted
int BinarySearch::search(const std::vector<int>& a, int t, std::vector<SearchStep>& s) const {
    int lo = 0, hi = int(a.size()) - 1;
    while (lo <= hi) {
        int mid = lo + (hi - lo) / 2;          // no overflow of lo + hi
        bool f = a[mid] == t;
        s.push_back({mid, lo, hi, f});
        if (f) {
            return mid;
        }
        if (a[mid] < t) {
            lo = mid + 1;
        } else {
            hi = mid - 1;
        }
    }
    return -1;
}

// split the range by Fibonacci numbers F(k-2) : F(k-1) instead of halves;
// needs only addition and subtraction
int FibonacciSearch::search(const std::vector<int>& a, int t, std::vector<SearchStep>& s) const {
    int n = int(a.size());
    int f2 = 0, f1 = 1, f = 1;                 // F(k-2), F(k-1), F(k)
    while (f < n) {
        f2 = f1;
        f1 = f;
        f = f1 + f2;
    }
    int offset = -1;                           // everything <= offset is eliminated
    while (f > 1) {
        int i = std::min(offset + f2, n - 1);
        bool found = a[i] == t;
        s.push_back({i, offset + 1, n - 1, found});
        if (found) {
            return i;
        }
        if (a[i] < t) {
            f = f1;
            f1 = f2;
            f2 = f - f1;
            offset = i;
        }   // go right: k-1
        else {
            f = f2;
            f1 = f1 - f2;
            f2 = f - f1;
        }                   // go left: k-2
    }
    if (f1 && offset + 1 < n) {
        bool found = a[offset + 1] == t;
        s.push_back({offset + 1, offset + 1, n - 1, found});
        if (found) {
            return offset + 1;
        }
    }
    return -1;
}

} // namespace vas::ds
