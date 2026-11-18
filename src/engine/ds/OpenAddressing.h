// =====================================================================
//  Open addressing: every key lives in the slot array itself  [PL CO5]
//  On a collision the next slot is tried by a probe sequence:
//     linear     index = (h(k) + i)   mod m
//     quadratic  index = (h(k) + i^2) mod m
//  The table grows (rehash to the next prime > 2m) above load 0.7.
// =====================================================================
#pragma once
#include "engine/ds/HashTable.h"

namespace vas::ds {

class OpenAddressingTable : public HashTable {
public:
    explicit OpenAddressingTable(int cap) : slots_(nextPrime(cap)) {}

    bool insert(const std::string& key, int value, ProbeTrace* t) override;
    bool find(const std::string& key, int& value, ProbeTrace* t) const override;
    int capacity() const override { return int(slots_.size()); }
    int size() const override { return count_; }
    void clear() override;
    std::vector<Bucket> buckets() const override;

protected:
    virtual int probeIndex(int home, int i) const = 0;   // i-th slot to try

private:
    struct Slot {
        std::string key;
        int value = 0;
        bool used = false;
    };
    bool place(const std::string& key, int value, ProbeTrace& tr);
    void rehash(int newCap);

    std::vector<Slot> slots_;
    int count_ = 0;
};

class LinearProbingTable : public OpenAddressingTable {
public:
    explicit LinearProbingTable(int cap = 7) : OpenAddressingTable(cap) {}
    std::string name() const override { return "Linear Probing"; }
    std::string formula() const override { return "index = (h(k) + i) mod m"; }
protected:
    int probeIndex(int home, int i) const override;
};

class QuadraticProbingTable : public OpenAddressingTable {
public:
    explicit QuadraticProbingTable(int cap = 7) : OpenAddressingTable(cap) {}
    std::string name() const override { return "Quadratic Probing"; }
    std::string formula() const override { return "index = (h(k) + i^2) mod m"; }
protected:
    int probeIndex(int home, int i) const override;
};


} // namespace vas::ds
