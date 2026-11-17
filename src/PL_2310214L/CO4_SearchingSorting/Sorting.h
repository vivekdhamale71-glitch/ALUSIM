// =====================================================================
//  Sorting algorithms for the Data Lab                       [PL CO4]
//  Sorter hierarchy (Bubble / Insertion / Quick): each sort records
//  every compare and swap so the Data Lab can animate the profiler's
//  instruction counts.                                       [PSOOP]
//  The generic quickSort<T> that sorts the opcode table is in
//  TableSort.h.
// =====================================================================
#pragma once
#include <string>
#include <vector>

#include "PL_2310214L/CO4_SearchingSorting/TableSort.h"

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
    void sort(std::vector<int>& a, std::vector<SortStep>& s) const override;
};

class InsertionSort : public Sorter {
public:
    std::string name() const override { return "Insertion Sort"; }
    std::string complexity() const override { return "O(n^2), fast on nearly sorted"; }
    void sort(std::vector<int>& a, std::vector<SortStep>& s) const override;
};

class QuickSort : public Sorter {
public:
    std::string name() const override { return "Quick Sort"; }
    std::string complexity() const override { return "O(n log n) average"; }
    void sort(std::vector<int>& a, std::vector<SortStep>& s) const override;

private:
    static void qs(std::vector<int>& a, int lo, int hi, std::vector<SortStep>& s);
};

} // namespace vas::ds
