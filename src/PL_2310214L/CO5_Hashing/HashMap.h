// =====================================================================
//  HashMap<K,V>  -  generic separate-chaining hash map  [PL CO5, PSOOP]
//  Template container used where the program needs fast lookup:
//    * opcode dispatch   mnemonic -> factory creator  (InstructionFactory)
//    * register names    "rax" -> 0, "r15" -> 15     (Assembler)
//    * sparse memory     address -> qword            (CPU)
//    * micro-code ROM    mnemonic -> micro address   (Microprogrammed CU)
//    * translation cache statement text -> assembly  (Compiler)
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
struct Hasher<std::string> {
    std::size_t operator()(const std::string& s) const {
        std::size_t h = 1469598103934665603ULL;           // FNV-1a
        for (unsigned char c : s) { h ^= c; h *= 1099511628211ULL; }
        return h;
    }
};

template <typename K>
struct Hasher<K, std::enable_if_t<std::is_integral_v<K>>> {
    std::size_t operator()(K k) const {                   // Knuth multiplicative
        return static_cast<std::size_t>(static_cast<std::uint64_t>(k) * 11400714819323198485ULL >> 17);
    }
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
    HashMap(const HashMap& o) : b_(o.b_.size(), nullptr) {
        o.forEach([this](const K& k, const V& v) { put(k, v); });
    }
    HashMap(HashMap&& o) noexcept : b_(16, nullptr) { swap(o); }
    HashMap& operator=(HashMap o) noexcept { swap(o); return *this; }
    ~HashMap() { clear(); }

    void put(const K& k, const V& v) {
        std::size_t i = index(k);
        for (Node* n = b_[i]; n; n = n->next)
            if (n->key == k) { n->value = v; return; }
        b_[i] = new Node{k, v, b_[i]};
        if (++n_ > b_.size()) rehash(b_.size() * 2);
    }
    V* find(const K& k) {
        for (Node* n = b_[index(k)]; n; n = n->next) if (n->key == k) return &n->value;
        return nullptr;
    }
    const V* find(const K& k) const {
        for (Node* n = b_[index(k)]; n; n = n->next) if (n->key == k) return &n->value;
        return nullptr;
    }
    bool get(const K& k, V& out) const {
        const V* p = find(k);
        if (!p) return false;
        out = *p;
        return true;
    }
    bool contains(const K& k) const { return find(k) != nullptr; }
    bool erase(const K& k) {
        Node** pp = &b_[index(k)];
        while (*pp) {
            if ((*pp)->key == k) { Node* d = *pp; *pp = d->next; delete d; --n_; return true; }
            pp = &(*pp)->next;
        }
        return false;
    }
    template <typename F>
    void forEach(F f) const {
        for (Node* h : b_) for (Node* n = h; n; n = n->next) f(n->key, n->value);
    }
    void clear() {
        for (auto& h : b_) while (h) { Node* nx = h->next; delete h; h = nx; }
        n_ = 0;
    }
    std::size_t size() const { return n_; }
    std::size_t bucketCount() const { return b_.size(); }

    void swap(HashMap& o) noexcept {
        b_.swap(o.b_);
        std::swap(n_, o.n_);
    }

private:
    std::size_t index(const K& k) const { return H{}(k) % b_.size(); }
    void rehash(std::size_t nb) {
        std::vector<Node*> old(nb, nullptr);
        old.swap(b_);
        for (Node* h : old) {
            while (h) {
                Node* nx = h->next;
                std::size_t i = index(h->key);
                h->next = b_[i];
                b_[i] = h;
                h = nx;
            }
        }
    }

    std::vector<Node*> b_;
    std::size_t n_ = 0;
};

} // namespace vas::ds
