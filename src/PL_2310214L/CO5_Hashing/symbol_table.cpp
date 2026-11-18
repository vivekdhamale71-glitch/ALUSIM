// A hash table with chaining that maps variable names to addresses and grows by rehashing when it fills up.

#include <iostream>
#include <string>

struct Entry {
    std::string name;
    long long address;
    Entry* next;
};

// h(s) = s0*31^(n-1) + ... + s(n-1)
unsigned long long hashOf(const std::string& key) {
    unsigned long long h = 0;
    for (std::size_t i = 0; i < key.size(); ++i) {
        h = h * 31 + static_cast<unsigned char>(key[i]);
    }
    return h;
}

bool isPrime(int n) {
    if (n < 2) {
        return false;
    }
    for (int d = 2; d * d <= n; ++d) {
        if (n % d == 0) {
            return false;
        }
    }
    return true;
}

// prime table sizes spread the keys more evenly
int nextPrime(int n) {
    while (!isPrime(n)) {
        n = n + 1;
    }
    return n;
}

class SymbolTable {
public:
    SymbolTable() : size_(0), count_(0), nextAddress_(0x2000), rehashes_(0) {
        size_ = 7;
        slots_ = new Entry*[size_];
        for (int i = 0; i < size_; ++i) {
            slots_[i] = nullptr;
        }
    }

    ~SymbolTable() {
        freeAll();
        delete[] slots_;
    }

    // adds a variable (once); returns its address
    long long declare(const std::string& name) {
        Entry* existing = find(name);
        if (existing != nullptr) {
            return existing->address;
        }
        if (count_ + 1 > 2 * size_) {
            grow();                                     // chains got too long
        }
        Entry* entry = new Entry;
        entry->name = name;
        entry->address = nextAddress_;
        nextAddress_ = nextAddress_ + 8;                // every variable is one 8-byte word
        int slot = static_cast<int>(hashOf(name) % size_);
        entry->next = slots_[slot];
        slots_[slot] = entry;
        count_ = count_ + 1;
        return entry->address;
    }

    // the address of a variable, or -1 if it was never declared
    long long lookup(const std::string& name) const {
        Entry* entry = find(name);
        if (entry == nullptr) {
            return -1;
        }
        return entry->address;
    }

    int size() const {
        return size_;
    }

    int count() const {
        return count_;
    }

    int rehashes() const {
        return rehashes_;
    }

    // length of the longest chain: what a lookup costs in the worst case
    int longestChain() const {
        int longest = 0;
        for (int i = 0; i < size_; ++i) {
            int length = 0;
            for (Entry* e = slots_[i]; e != nullptr; e = e->next) {
                length = length + 1;
            }
            if (length > longest) {
                longest = length;
            }
        }
        return longest;
    }

    void print() const {
        for (int i = 0; i < size_; ++i) {
            if (slots_[i] == nullptr) {
                continue;
            }
            std::cout << "    slot " << i << ":";
            for (Entry* e = slots_[i]; e != nullptr; e = e->next) {
                std::cout << " " << e->name;
            }
            std::cout << "\n";
        }
    }

private:
    Entry* find(const std::string& name) const {
        int slot = static_cast<int>(hashOf(name) % size_);
        for (Entry* e = slots_[slot]; e != nullptr; e = e->next) {
            if (e->name == name) {
                return e;
            }
        }
        return nullptr;
    }

    // a bigger prime-sized array, every entry moved to the slot it now belongs to
    void grow() {
        int newSize = nextPrime(size_ * 2 + 1);
        Entry** bigger = new Entry*[newSize];
        for (int i = 0; i < newSize; ++i) {
            bigger[i] = nullptr;
        }
        for (int i = 0; i < size_; ++i) {
            Entry* e = slots_[i];
            while (e != nullptr) {
                Entry* following = e->next;
                int slot = static_cast<int>(hashOf(e->name) % newSize);
                e->next = bigger[slot];
                bigger[slot] = e;
                e = following;
            }
        }
        delete[] slots_;
        slots_ = bigger;
        size_ = newSize;
        rehashes_ = rehashes_ + 1;
    }

    void freeAll() {
        for (int i = 0; i < size_; ++i) {
            while (slots_[i] != nullptr) {
                Entry* following = slots_[i]->next;
                delete slots_[i];
                slots_[i] = following;
            }
        }
    }

    Entry** slots_;
    int size_;
    int count_;
    long long nextAddress_;
    int rehashes_;
};
