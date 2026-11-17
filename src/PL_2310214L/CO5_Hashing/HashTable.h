// =====================================================================
//  HashTable  -  string -> int symbol tables                 [PL CO5]
//  Abstract base class; the collision strategies are subclasses:
//     LinearProbingTable    h(k) + i       (OpenAddressing.h)
//     QuadraticProbingTable h(k) + i^2     (OpenAddressing.h)
//     ChainingTable         separate chaining (Chaining.h)
//  Used by the compiler's variable symbol table (name -> address) and
//  the assembler's label table (label -> instruction index); the user
//  picks the method. Every insert / find can fill a ProbeTrace so the
//  Data Lab can animate it.
// =====================================================================
#pragma once
#include <string>
#include <utility>
#include <vector>

namespace vas::ds {

struct ProbeTrace {
    std::string key;
    unsigned long long rawHash = 0;   // polynomial hash before "mod"
    int capacity = 0;                 // table size used for the mod
    int home = -1;                    // rawHash % capacity
    std::vector<int> probes;          // every slot that was examined
    int finalIndex = -1;              // where the key ended up
    int chainPos = -1;                // position inside the chain (chaining)
    bool collision = false;
    bool rehashed = false;            // table grew before this insert
    int oldCapacity = 0;
    bool found = false;
};

// h(s) = s0 * 31^(n-1) + s1 * 31^(n-2) + ... + s(n-1)
unsigned long long polyHash(const std::string& s);
bool isPrime(int n);
int nextPrime(int n);                 // table sizes are prime: fewer collisions

using Bucket = std::vector<std::pair<std::string, int>>;

class HashTable {
public:
    virtual ~HashTable() = default;
    virtual std::string name() const = 0;
    virtual std::string formula() const = 0;
    virtual bool insert(const std::string& key, int value, ProbeTrace* t = nullptr) = 0;
    virtual bool find(const std::string& key, int& value, ProbeTrace* t = nullptr) const = 0;
    virtual int capacity() const = 0;
    virtual int size() const = 0;
    virtual void clear() = 0;
    // one bucket per slot; probing tables hold 0 or 1 entry per bucket
    virtual std::vector<Bucket> buckets() const = 0;

    double loadFactor() const { return capacity() ? double(size()) / capacity() : 0.0; }
    int totalProbes() const { return probes_; }
    int collisions() const { return collisions_; }

protected:
    mutable int probes_ = 0;
    mutable int collisions_ = 0;
};

enum class HashMethod { Linear, Quadratic, Chaining };

// factory: the strategy the user chose in the UI
HashTable* makeHashTable(HashMethod m, int cap = 7);

} // namespace vas::ds

#include "PL_2310214L/CO5_Hashing/OpenAddressing.h"
#include "PL_2310214L/CO5_Hashing/Chaining.h"
