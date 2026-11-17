// =====================================================================
//  Searching                                                 [PL CO4]
//  * Searcher hierarchy (Linear / Binary / Fibonacci) records each
//    probe so the Data Lab memory inspector can animate the search and
//    compare how many comparisons each method needed.       [PSOOP]
//  * binarySearch<T> template is used by the program itself:
//      - the lexer looks up keywords (if / else / while)
//      - the assembler looks up mnemonics in the sorted opcode table
// =====================================================================
#pragma once
#include <algorithm>
#include <string>
#include <vector>

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
    int search(const std::vector<int>& a, int t, std::vector<SearchStep>& s) const override {
        for (int i = 0; i < int(a.size()); ++i) {
            bool f = a[i] == t;
            s.push_back({i, 0, int(a.size()) - 1, f});
            if (f) return i;
        }
        return -1;
    }
};

class BinarySearch : public Searcher {
public:
    std::string name() const override { return "Binary Search"; }
    std::string complexity() const override { return "O(log n), needs sorted data"; }
    bool needsSorted() const override { return true; }
    int search(const std::vector<int>& a, int t, std::vector<SearchStep>& s) const override {
        int lo = 0, hi = int(a.size()) - 1;
        while (lo <= hi) {
            int mid = lo + (hi - lo) / 2;
            bool f = a[mid] == t;
            s.push_back({mid, lo, hi, f});
            if (f) return mid;
            if (a[mid] < t) lo = mid + 1; else hi = mid - 1;
        }
        return -1;
    }
};

class FibonacciSearch : public Searcher {
public:
    std::string name() const override { return "Fibonacci Search"; }
    std::string complexity() const override { return "O(log n), only + and -"; }
    bool needsSorted() const override { return true; }
    int search(const std::vector<int>& a, int t, std::vector<SearchStep>& s) const override {
        int n = int(a.size());
        int f2 = 0, f1 = 1, f = 1;
        while (f < n) { f2 = f1; f1 = f; f = f1 + f2; }
        int offset = -1;
        while (f > 1) {
            int i = std::min(offset + f2, n - 1);
            bool found = a[i] == t;
            s.push_back({i, offset + 1, n - 1, found});
            if (found) return i;
            if (a[i] < t) { f = f1; f1 = f2; f2 = f - f1; offset = i; }
            else { f = f2; f1 = f1 - f2; f2 = f - f1; }
        }
        if (f1 && offset + 1 < n) {
            bool found = a[offset + 1] == t;
            s.push_back({offset + 1, offset + 1, n - 1, found});
            if (found) return offset + 1;
        }
        return -1;
    }
};

// cmp(element, key) returns <0, 0, >0. `probes` collects visited indices.
template <typename T, typename K, typename Cmp>
int binarySearch(const std::vector<T>& a, const K& key, Cmp cmp, std::vector<int>* probes = nullptr) {
    int lo = 0, hi = int(a.size()) - 1;
    while (lo <= hi) {
        int mid = lo + (hi - lo) / 2;
        if (probes) probes->push_back(mid);
        int c = cmp(a[mid], key);
        if (c == 0) return mid;
        if (c < 0) lo = mid + 1; else hi = mid - 1;
    }
    return -1;
}

} // namespace vas::ds
