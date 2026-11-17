// =====================================================================
//  Sorting                                                   [PL CO4]
//  * Sorter hierarchy (Bubble / Insertion / Quick) records every
//    compare and swap so the Data Lab can animate the profiler data
//    (instruction frequencies) and the symbol table values.  [PSOOP]
//  * quickSort<T> template does the real work at start-up: it sorts
//    the opcode table so the assembler can binary-search it.
// =====================================================================
#pragma once
#include <string>
#include <utility>
#include <vector>

namespace vas::ds {

struct SortStep {
    enum Type { Compare, Swap, Pivot, Sorted, Done } type;
    int i = -1;
    int j = -1;
};

class Sorter {
public:
    virtual ~Sorter() = default;
    virtual std::string name() const = 0;
    virtual std::string complexity() const = 0;
    virtual void sort(std::vector<int>& a, std::vector<SortStep>& steps) const = 0;
};

class BubbleSort : public Sorter {
public:
    std::string name() const override { return "Bubble Sort"; }
    std::string complexity() const override { return "O(n^2) compares, stable"; }
    void sort(std::vector<int>& a, std::vector<SortStep>& s) const override {
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
                for (int k = 0; k < n - 1 - pass; ++k) s.push_back({SortStep::Sorted, k, -1});
                break;
            }
        }
        if (n) s.push_back({SortStep::Sorted, 0, -1});
        s.push_back({SortStep::Done});
    }
};

class InsertionSort : public Sorter {
public:
    std::string name() const override { return "Insertion Sort"; }
    std::string complexity() const override { return "O(n^2), fast on nearly sorted"; }
    void sort(std::vector<int>& a, std::vector<SortStep>& s) const override {
        int n = int(a.size());
        for (int i = 1; i < n; ++i) {
            int j = i;
            while (j > 0) {
                s.push_back({SortStep::Compare, j - 1, j});
                if (a[j - 1] <= a[j]) break;
                std::swap(a[j - 1], a[j]);
                s.push_back({SortStep::Swap, j - 1, j});
                --j;
            }
        }
        for (int k = 0; k < n; ++k) s.push_back({SortStep::Sorted, k, -1});
        s.push_back({SortStep::Done});
    }
};

class QuickSort : public Sorter {
public:
    std::string name() const override { return "Quick Sort"; }
    std::string complexity() const override { return "O(n log n) average"; }
    void sort(std::vector<int>& a, std::vector<SortStep>& s) const override {
        if (!a.empty()) qs(a, 0, int(a.size()) - 1, s);
        s.push_back({SortStep::Done});
    }

private:
    static void qs(std::vector<int>& a, int lo, int hi, std::vector<SortStep>& s) {
        if (lo > hi) return;
        if (lo == hi) { s.push_back({SortStep::Sorted, lo, -1}); return; }
        int pivot = a[hi];
        s.push_back({SortStep::Pivot, hi, -1});
        int i = lo;
        for (int j = lo; j < hi; ++j) {
            s.push_back({SortStep::Compare, j, hi});
            if (a[j] < pivot) {
                if (i != j) { std::swap(a[i], a[j]); s.push_back({SortStep::Swap, i, j}); }
                ++i;
            }
        }
        if (i != hi) { std::swap(a[i], a[hi]); s.push_back({SortStep::Swap, i, hi}); }
        s.push_back({SortStep::Sorted, i, -1});
        qs(a, lo, i - 1, s);
        qs(a, i + 1, hi, s);
    }
};

// generic quick sort used by the program itself (opcode table, listings)
template <typename T, typename Less>
void quickSort(std::vector<T>& a, Less less, int lo = 0, int hi = -2) {
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
