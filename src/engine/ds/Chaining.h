// =====================================================================
//  Separate chaining: each slot heads a linked list of the keys
//  that hash to it                                           [PL CO5]
//     index = h(k) mod m, then walk / append to that chain
//  The table grows when the average chain length passes 2.
// =====================================================================
#pragma once
#include "engine/ds/HashTable.h"

namespace vas::ds {

class ChainingTable : public HashTable {
    struct Node {
        std::string key;
        int value;
        Node* next;
    };

public:
    explicit ChainingTable(int cap = 7) : heads_(nextPrime(cap), nullptr) {}
    ChainingTable(const ChainingTable&) = delete;
    ChainingTable& operator=(const ChainingTable&) = delete;
    ~ChainingTable() override { freeAll(); }

    std::string name() const override { return "Separate Chaining"; }
    std::string formula() const override { return "index = h(k) mod m, append to chain"; }
    bool insert(const std::string& key, int value, ProbeTrace* t) override;
    bool find(const std::string& key, int& value, ProbeTrace* t) const override;
    int capacity() const override { return int(heads_.size()); }
    int size() const override { return count_; }
    void clear() override {
        freeAll();
        probes_ = collisions_ = 0;
    }
    std::vector<Bucket> buckets() const override;

private:
    void freeAll();
    void rehash(int newCap);

    std::vector<Node*> heads_;
    int count_ = 0;
};

} // namespace vas::ds
