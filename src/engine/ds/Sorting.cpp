#include "engine/ds/Sorting.h"

#include <utility>

namespace vas::ds {

// each pass bubbles the largest remaining value to the end; stop early
// when a pass makes no swap
void BubbleSort::sort(std::vector<int>& a, std::vector<SortStep>& s) const {
    int n = int(a.size());
    for (int pass = 0; pass < n - 1; ++pass) {
        bool swapped = false;
        for (int j = 0; j < n - 1 - pass; ++j) {
            s.push_back({SortStep::Compare, j, j + 1});
            if (a[j] > a[j + 1]) {
                std::swap(a[j], a[j + 1]);
                s.push_back({SortStep::Swap, j, j + 1});
                swapped = true;
            }
        }
        s.push_back({SortStep::Sorted, n - 1 - pass, -1});
        if (!swapped) {
            for (int k = 0; k < n - 1 - pass; ++k) {
                s.push_back({SortStep::Sorted, k, -1});
            }
            break;
        }
    }
    if (n) {
        s.push_back({SortStep::Sorted, 0, -1});
    }
    s.push_back({SortStep::Done});
}

// grow a sorted prefix: move a[i] left until its neighbour is not larger
void InsertionSort::sort(std::vector<int>& a, std::vector<SortStep>& s) const {
    int n = int(a.size());
    for (int i = 1; i < n; ++i) {
        int j = i;
        while (j > 0) {
            s.push_back({SortStep::Compare, j - 1, j});
            if (a[j - 1] <= a[j]) {
                break;
            }
            std::swap(a[j - 1], a[j]);
            s.push_back({SortStep::Swap, j - 1, j});
            --j;
        }
    }
    for (int k = 0; k < n; ++k) {
        s.push_back({SortStep::Sorted, k, -1});
    }
    s.push_back({SortStep::Done});
}

void QuickSort::sort(std::vector<int>& a, std::vector<SortStep>& s) const {
    if (!a.empty()) {
        qs(a, 0, int(a.size()) - 1, s);
    }
    s.push_back({SortStep::Done});
}

// Lomuto partition around the last element, then sort both sides
void QuickSort::qs(std::vector<int>& a, int lo, int hi, std::vector<SortStep>& s) {
    if (lo > hi) {
        return;
    }
    if (lo == hi) {
        s.push_back({SortStep::Sorted, lo, -1});
        return;
    }
    int pivot = a[hi];
    s.push_back({SortStep::Pivot, hi, -1});
    int i = lo;                                // a[lo .. i-1] < pivot
    for (int j = lo; j < hi; ++j) {
        s.push_back({SortStep::Compare, j, hi});
        if (a[j] < pivot) {
            if (i != j) {
                std::swap(a[i], a[j]);
                s.push_back({SortStep::Swap, i, j});
            }
            ++i;
        }
    }
    if (i != hi) {
        std::swap(a[i], a[hi]);
        s.push_back({SortStep::Swap, i, hi});
    }
    s.push_back({SortStep::Sorted, i, -1});    // the pivot is in its final place
    qs(a, lo, i - 1, s);
    qs(a, i + 1, hi, s);
}

} // namespace vas::ds
