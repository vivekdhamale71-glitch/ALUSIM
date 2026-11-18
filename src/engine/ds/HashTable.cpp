#include "engine/ds/HashTable.h"

namespace vas::ds {

unsigned long long polyHash(const std::string& s) {
    unsigned long long h = 0;
    for (unsigned char c : s) {
        h = h * 31ULL + c;
    }    // Horner's rule
    return h;
}

bool isPrime(int n) {
    if (n < 2) {
        return false;
    }
    for (int i = 2; i * i <= n; ++i) {
        if (n % i == 0) {
            return false;
        }
    }
    return true;
}

int nextPrime(int n) {
    while (!isPrime(n)) {
        ++n;
    }
    return n;
}

HashTable* makeHashTable(HashMethod m, int cap) {
    switch (m) {
        case HashMethod::Linear: return new LinearProbingTable(cap);
        case HashMethod::Quadratic: return new QuadraticProbingTable(cap);
        default: return new ChainingTable(cap);
    }
}

} // namespace vas::ds
