// =====================================================================
//  HashMap<K,V>  -  generic separate-chaining hash map  [PL CO5, PSOOP]
//  Template container used where the program needs fast lookup:
//    * opcode dispatch   mnemonic -> factory creator  (InstructionFactory)
//    * register names    "rax" -> 0, "r15" -> 15     (Assembler)
//    * sparse memory     address -> qword            (CPU)
//    * micro-code ROM    mnemonic -> micro address   (Microprogrammed CU)
//    * translation cache statement text -> assembly  (Compiler)
//  Average O(1) put / find; doubles its buckets when load > 1.
//  Member functions are defined in HashMap.cpp (included at the end).
// =====================================================================
#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace vas::ds {

template <typename K, typename Enable = void>
struct Hasher;

template <>
struct Hasher<std::string> {                  // FNV-1a
    std::size_t operator()(const std::string& s) const;
};

template <typename K>
struct Hasher<K, std::enable_if_t<std::is_integral_v<K>>> {   // Knuth multiplicative
    std::size_t operator()(K k) const;
};

template <typename K, typename V, typename H = Hasher<K>>
class HashMap {
    struct Node {
        K key;
        V value;
        Node* next;
    };

public:
    explicit HashMap(std::size_t buckets = 16) : b_(buckets ? buckets : 16, nullptr) {}
    HashMap(const HashMap& o);
    HashMap(HashMap&& o) noexcept : b_(16, nullptr) { swap(o); }
    HashMap& operator=(HashMap o) noexcept { swap(o); return *this; }
    ~HashMap() { clear(); }

    void put(const K& k, const V& v);
    V* find(const K& k);
    const V* find(const K& k) const;
    bool get(const K& k, V& out) const;
    bool contains(const K& k) const { return find(k) != nullptr; }
    bool erase(const K& k);
    template <typename F>
    void forEach(F f) const {
        for (Node* h : b_)
            for (Node* n = h; n; n = n->next) f(n->key, n->value);
    }
    void clear();
    std::size_t size() const { return n_; }
    std::size_t bucketCount() const { return b_.size(); }
    void swap(HashMap& o) noexcept { b_.swap(o.b_); std::swap(n_, o.n_); }

private:
    std::size_t index(const K& k) const { return H{}(k) % b_.size(); }
    void rehash(std::size_t nb);

    std::vector<Node*> b_;
    std::size_t n_ = 0;
};

} // namespace vas::ds

#include "PL_2310214L/CO5_Hashing/HashMap.cpp"
