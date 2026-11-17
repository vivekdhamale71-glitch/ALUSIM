// =====================================================================
//  Searching algorithms for the Data Lab                     [PL CO4]
//  Searcher hierarchy (Linear / Binary / Fibonacci): each search
//  records every probe so the memory inspector can animate it and
//  compare how many comparisons each method needed.         [PSOOP]
//  The generic binarySearch<T> used by the lexer and the assembler is
//  in TableSearch.h.
// =====================================================================
#pragma once
#include <string>
#include <vector>

#include "PL_2310214L/CO4_SearchingSorting/TableSearch.h"

namespace vas::ds {

struct SearchStep {
    int probe = -1;   // index compared in this step
    int lo = -1;
    int hi = -1;
    bool found = false;
};

class Searcher {
public:
    virtual ~Searcher() = default;
    virtual std::string name() const = 0;
    virtual std::string complexity() const = 0;
    virtual bool needsSorted() const = 0;
    virtual int search(const std::vector<int>& a, int target, std::vector<SearchStep>& steps) const = 0;
};

class LinearSearch : public Searcher {
public:
    std::string name() const override { return "Linear Search"; }
    std::string complexity() const override { return "O(n)"; }
    bool needsSorted() const override { return false; }
    int search(const std::vector<int>& a, int t, std::vector<SearchStep>& s) const override;
};

class BinarySearch : public Searcher {
public:
    std::string name() const override { return "Binary Search"; }
    std::string complexity() const override { return "O(log n), needs sorted data"; }
    bool needsSorted() const override { return true; }
    int search(const std::vector<int>& a, int t, std::vector<SearchStep>& s) const override;
};

class FibonacciSearch : public Searcher {
public:
    std::string name() const override { return "Fibonacci Search"; }
    std::string complexity() const override { return "O(log n), only + and -"; }
    bool needsSorted() const override { return true; }
    int search(const std::vector<int>& a, int t, std::vector<SearchStep>& s) const override;
};

} // namespace vas::ds
