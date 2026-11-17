// =====================================================================
//  HashTable  -  string -> int symbol tables                 [PL CO5]
//  Abstract base + three collision strategies               [PSOOP]
//     LinearProbingTable    h(k)+i
//     QuadraticProbingTable h(k)+i^2
//     ChainingTable         separate chaining (linked nodes)
//  Used by: the compiler's variable symbol table (name -> address) and
//  the assembler's label table (label -> instruction index). Every
//  insert/find can record a ProbeTrace so the UI can animate it.
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

inline unsigned long long polyHash(const std::string& s) {
    unsigned long long h = 0;
    for (unsigned char c : s) h = h * 31ULL + c;
    return h;
}

inline bool isPrime(int n) {
    if (n < 2) return false;
    for (int i = 2; i * i <= n; ++i) if (n % i == 0) return false;
    return true;
}
inline int nextPrime(int n) { while (!isPrime(n)) ++n; return n; }

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

// ------------------------------------------------------- open addressing
class OpenAddressingTable : public HashTable {
public:
    explicit OpenAddressingTable(int cap) : slots_(nextPrime(cap)) {}

    bool insert(const std::string& key, int value, ProbeTrace* t) override {
        ProbeTrace local;
        ProbeTrace& tr = t ? *t : local;
        tr = ProbeTrace{};
        tr.key = key;
        // keep the load factor below 0.7 so probing stays short
        if ((count_ + 1) > 0.7 * capacity()) {
            tr.rehashed = true;
            tr.oldCapacity = capacity();
            rehash(nextPrime(capacity() * 2 + 1));
        }
        while (true) {
            if (place(key, value, tr)) return true;
            tr.rehashed = true;                  // quadratic probe found no slot
            tr.oldCapacity = capacity();
            rehash(nextPrime(capacity() * 2 + 1));
        }
    }

    bool find(const std::string& key, int& value, ProbeTrace* t) const override {
        ProbeTrace local;
        ProbeTrace& tr = t ? *t : local;
        tr = ProbeTrace{};
        tr.key = key;
        tr.rawHash = polyHash(key);
        tr.capacity = capacity();
        tr.home = int(tr.rawHash % capacity());
        for (int i = 0; i < capacity(); ++i) {
            int idx = probeIndex(tr.home, i);
            tr.probes.push_back(idx);
            ++probes_;
            if (!slots_[idx].used) return false;
            if (slots_[idx].key == key) {
                value = slots_[idx].value;
                tr.finalIndex = idx;
                tr.found = true;
                return true;
            }
        }
        return false;
    }

    int capacity() const override { return int(slots_.size()); }
    int size() const override { return count_; }
    void clear() override {
        for (auto& s : slots_) s = Slot{};
        count_ = 0;
        probes_ = collisions_ = 0;
    }
    std::vector<Bucket> buckets() const override {
        std::vector<Bucket> b(slots_.size());
        for (std::size_t i = 0; i < slots_.size(); ++i)
            if (slots_[i].used) b[i].push_back({slots_[i].key, slots_[i].value});
        return b;
    }

protected:
    virtual int probeIndex(int home, int i) const = 0;

private:
    struct Slot {
        std::string key;
        int value = 0;
        bool used = false;
    };

    bool place(const std::string& key, int value, ProbeTrace& tr) {
        tr.probes.clear();
        tr.rawHash = polyHash(key);
        tr.capacity = capacity();
        tr.home = int(tr.rawHash % capacity());
        for (int i = 0; i < capacity(); ++i) {
            int idx = probeIndex(tr.home, i);
            tr.probes.push_back(idx);
            ++probes_;
            Slot& s = slots_[idx];
            if (s.used && s.key == key) { s.value = value; tr.finalIndex = idx; tr.found = true; return true; }
            if (!s.used) {
                s.key = key; s.value = value; s.used = true;
                ++count_;
                tr.finalIndex = idx;
                return true;
            }
            if (i == 0) { tr.collision = true; ++collisions_; }
        }
        return false;
    }

    void rehash(int newCap) {
        std::vector<Slot> old = std::move(slots_);
        slots_.assign(newCap, Slot{});
        count_ = 0;
        int p = probes_, c = collisions_;
        ProbeTrace dummy;
        for (auto& s : old) if (s.used) place(s.key, s.value, dummy);
        probes_ = p; collisions_ = c;            // rehash work is not user probes
    }

    std::vector<Slot> slots_;
    int count_ = 0;
};

class LinearProbingTable : public OpenAddressingTable {
public:
    explicit LinearProbingTable(int cap = 7) : OpenAddressingTable(cap) {}
    std::string name() const override { return "Linear Probing"; }
    std::string formula() const override { return "index = (h(k) + i) mod m"; }
protected:
    int probeIndex(int home, int i) const override { return (home + i) % capacity(); }
};

class QuadraticProbingTable : public OpenAddressingTable {
public:
    explicit QuadraticProbingTable(int cap = 7) : OpenAddressingTable(cap) {}
    std::string name() const override { return "Quadratic Probing"; }
    std::string formula() const override { return "index = (h(k) + i^2) mod m"; }
protected:
    int probeIndex(int home, int i) const override {
        return int((static_cast<long long>(home) + static_cast<long long>(i) * i) % capacity());
    }
};

// ------------------------------------------------------ separate chaining
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

    bool insert(const std::string& key, int value, ProbeTrace* t) override {
        ProbeTrace local;
        ProbeTrace& tr = t ? *t : local;
        tr = ProbeTrace{};
        tr.key = key;
        if (count_ + 1 > 2 * capacity()) {            // chains too long -> grow
            tr.rehashed = true;
            tr.oldCapacity = capacity();
            rehash(nextPrime(capacity() * 2 + 1));
        }
        tr.rawHash = polyHash(key);
        tr.capacity = capacity();
        tr.home = int(tr.rawHash % capacity());
        tr.finalIndex = tr.home;
        tr.probes.push_back(tr.home);
        ++probes_;
        Node** pp = &heads_[tr.home];
        int pos = 0;
        if (*pp) { tr.collision = true; ++collisions_; }
        while (*pp) {
            if ((*pp)->key == key) { (*pp)->value = value; tr.chainPos = pos; tr.found = true; return true; }
            pp = &(*pp)->next;
            ++pos;
        }
        *pp = new Node{key, value, nullptr};
        tr.chainPos = pos;
        ++count_;
        return true;
    }

    bool find(const std::string& key, int& value, ProbeTrace* t) const override {
        ProbeTrace local;
        ProbeTrace& tr = t ? *t : local;
        tr = ProbeTrace{};
        tr.key = key;
        tr.rawHash = polyHash(key);
        tr.capacity = capacity();
        tr.home = int(tr.rawHash % capacity());
        tr.probes.push_back(tr.home);
        ++probes_;
        int pos = 0;
        for (Node* n = heads_[tr.home]; n; n = n->next, ++pos) {
            if (n->key == key) {
                value = n->value; tr.finalIndex = tr.home; tr.chainPos = pos; tr.found = true;
                return true;
            }
        }
        return false;
    }

    int capacity() const override { return int(heads_.size()); }
    int size() const override { return count_; }
    void clear() override { freeAll(); probes_ = collisions_ = 0; }
    std::vector<Bucket> buckets() const override {
        std::vector<Bucket> b(heads_.size());
        for (std::size_t i = 0; i < heads_.size(); ++i)
            for (Node* n = heads_[i]; n; n = n->next) b[i].push_back({n->key, n->value});
        return b;
    }

private:
    void freeAll() {
        for (auto& h : heads_) {
            while (h) { Node* n = h->next; delete h; h = n; }
        }
        count_ = 0;
    }
    void rehash(int newCap) {
        std::vector<Node*> old = std::move(heads_);
        heads_.assign(newCap, nullptr);
        for (Node* h : old) {
            while (h) {
                Node* nx = h->next;
                int idx = int(polyHash(h->key) % newCap);
                h->next = nullptr;
                Node** pp = &heads_[idx];
                while (*pp) pp = &(*pp)->next;
                *pp = h;
                h = nx;
            }
        }
    }

    std::vector<Node*> heads_;
    int count_ = 0;
};

enum class HashMethod { Linear, Quadratic, Chaining };

inline HashTable* makeHashTable(HashMethod m, int cap = 7) {
    switch (m) {
        case HashMethod::Linear: return new LinearProbingTable(cap);
        case HashMethod::Quadratic: return new QuadraticProbingTable(cap);
        default: return new ChainingTable(cap);
    }
}

} // namespace vas::ds
