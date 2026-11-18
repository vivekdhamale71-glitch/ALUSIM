// Linear probing, quadratic probing, double hashing and chaining on the same keys, counting probes and collisions.

#include <iostream>
#include <string>

const int SLOTS = 17;       // a prime number of slots spreads the keys best

// h(s) = s0*31^(n-1) + s1*31^(n-2) + ... + s(n-1)   (computed with Horner's rule)
unsigned long long hashOf(const std::string& key) {
    unsigned long long h = 0;
    for (std::size_t i = 0; i < key.size(); ++i) {
        h = h * 31 + static_cast<unsigned char>(key[i]);
    }
    return h;
}

// ------------------------------------------------------------ open addressing
// every key lives in the slot array itself; the strategies differ only in which
// slot is tried after the home slot is taken
class OpenTable {
public:
    OpenTable() : probes_(0), collisions_(0) {
        for (int i = 0; i < SLOTS; ++i) {
            used_[i] = false;
        }
    }

    virtual ~OpenTable() {
    }

    virtual std::string name() const = 0;

    // the slot to try on attempt number 'attempt' (0 = the home slot)
    virtual int slotFor(const std::string& key, int attempt) const = 0;

    // stores a key; returns false if no free slot was found
    bool insert(const std::string& key) {
        for (int attempt = 0; attempt < SLOTS; ++attempt) {
            int slot = slotFor(key, attempt);
            probes_ = probes_ + 1;
            if (!used_[slot]) {
                used_[slot] = true;
                keys_[slot] = key;
                if (attempt > 0) {
                    collisions_ = collisions_ + 1;       // the home slot was already taken
                }
                return true;
            }
        }
        return false;
    }

    // follows the same sequence of slots as insert()
    bool find(const std::string& key) {
        for (int attempt = 0; attempt < SLOTS; ++attempt) {
            int slot = slotFor(key, attempt);
            probes_ = probes_ + 1;
            if (!used_[slot]) {
                return false;
            }
            if (keys_[slot] == key) {
                return true;
            }
        }
        return false;
    }

    int probes() const {
        return probes_;
    }

    int collisions() const {
        return collisions_;
    }

    void printLayout() const {
        for (int i = 0; i < SLOTS; ++i) {
            if (used_[i]) {
                std::cout << "       slot " << i << ": " << keys_[i] << "\n";
            }
        }
    }

private:
    bool used_[SLOTS];
    std::string keys_[SLOTS];
    int probes_;
    int collisions_;
};

class LinearTable : public OpenTable {
public:
    std::string name() const {
        return "linear probing";
    }
    int slotFor(const std::string& key, int attempt) const {
        int home = static_cast<int>(hashOf(key) % SLOTS);
        return (home + attempt) % SLOTS;
    }
};

class QuadraticTable : public OpenTable {
public:
    std::string name() const {
        return "quadratic probing";
    }
    int slotFor(const std::string& key, int attempt) const {
        int home = static_cast<int>(hashOf(key) % SLOTS);
        return (home + attempt * attempt) % SLOTS;
    }
};

class DoubleHashTable : public OpenTable {
public:
    std::string name() const {
        return "double hashing";
    }
    int slotFor(const std::string& key, int attempt) const {
        int home = static_cast<int>(hashOf(key) % SLOTS);
        int step = 1 + static_cast<int>(hashOf(key) % (SLOTS - 1));     // second hash: 1 .. 16, never 0
        return (home + attempt * step) % SLOTS;
    }
};

// ------------------------------------------------------------ separate chaining
struct ChainNode {
    std::string key;
    ChainNode* next;
};

// every slot holds the start of a linked list of the keys that hash to it
class ChainTable {
public:
    ChainTable() : probes_(0), collisions_(0) {
        for (int i = 0; i < SLOTS; ++i) {
            heads_[i] = nullptr;
        }
    }

    ~ChainTable() {
        for (int i = 0; i < SLOTS; ++i) {
            while (heads_[i] != nullptr) {
                ChainNode* following = heads_[i]->next;
                delete heads_[i];
                heads_[i] = following;
            }
        }
    }

    void insert(const std::string& key) {
        int slot = static_cast<int>(hashOf(key) % SLOTS);
        probes_ = probes_ + 1;
        if (heads_[slot] != nullptr) {
            collisions_ = collisions_ + 1;
        }
        ChainNode* node = new ChainNode;
        node->key = key;
        node->next = heads_[slot];                  // new keys join the front of the list
        heads_[slot] = node;
    }

    bool find(const std::string& key) {
        int slot = static_cast<int>(hashOf(key) % SLOTS);
        probes_ = probes_ + 1;
        for (ChainNode* node = heads_[slot]; node != nullptr; node = node->next) {
            if (node->key == key) {
                return true;
            }
            probes_ = probes_ + 1;
        }
        return false;
    }

    int probes() const {
        return probes_;
    }

    int collisions() const {
        return collisions_;
    }

private:
    ChainNode* heads_[SLOTS];
    int probes_;
    int collisions_;
};

// stores every key, looks every key up again, prints the totals; false if a key was lost
