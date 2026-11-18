// HashMap<K,V> member functions                          [PL CO5]
#ifndef VAS_HASHMAP_IMPL
#define VAS_HASHMAP_IMPL
#include "engine/ds/HashMap.h"

namespace vas::ds {

inline std::size_t Hasher<std::string>::operator()(const std::string& s) const {
    std::size_t h = 1469598103934665603ULL;
    for (unsigned char c : s) {
        h ^= c;
        h *= 1099511628211ULL;
    }
    return h;
}

template <typename K>
std::size_t Hasher<K>::operator()(K k) const {
    return static_cast<std::size_t>(static_cast<std::uint64_t>(k) * 11400714819323198485ULL >> 17);
}

template <typename K, typename V, typename H>
HashMap<K, V, H>::HashMap(std::size_t buckets) {
    if (buckets == 0) {
        buckets = 16;
    }
    b_.assign(buckets, nullptr);
}

// copy: insert every (key, value) of the other map into a table of the same size
template <typename K, typename V, typename H>
HashMap<K, V, H>::HashMap(const HashMap& o) : b_(o.b_.size(), nullptr) {
    for (std::size_t i = 0; i < o.b_.size(); ++i) {
        for (Node* n = o.b_[i]; n != nullptr; n = n->next) {
            put(n->key, n->value);
        }
    }
}

// insert or update: the new node goes to the front of its chain
template <typename K, typename V, typename H>
void HashMap<K, V, H>::put(const K& k, const V& v) {
    std::size_t i = index(k);
    for (Node* n = b_[i]; n; n = n->next) {
        if (n->key == k) {
            n->value = v;
            return;
        }
    }
    b_[i] = new Node{k, v, b_[i]};
    if (++n_ > b_.size()) {
        rehash(b_.size() * 2);
    }
}

template <typename K, typename V, typename H>
V* HashMap<K, V, H>::find(const K& k) {
    for (Node* n = b_[index(k)]; n; n = n->next) {
        if (n->key == k) {
            return &n->value;
        }
    }
    return nullptr;
}

template <typename K, typename V, typename H>
const V* HashMap<K, V, H>::find(const K& k) const {
    for (Node* n = b_[index(k)]; n; n = n->next) {
        if (n->key == k) {
            return &n->value;
        }
    }
    return nullptr;
}

template <typename K, typename V, typename H>
bool HashMap<K, V, H>::get(const K& k, V& out) const {
    const V* p = find(k);
    if (!p) {
        return false;
    }
    out = *p;
    return true;
}

// unlink the node from its chain
template <typename K, typename V, typename H>
bool HashMap<K, V, H>::erase(const K& k) {
    Node** pp = &b_[index(k)];
    while (*pp) {
        if ((*pp)->key == k) {
            Node* d = *pp;
            *pp = d->next;
            delete d;
            --n_;
            return true;
        }
        pp = &(*pp)->next;
    }
    return false;
}

template <typename K, typename V, typename H>
void HashMap<K, V, H>::clear() {
    for (std::size_t i = 0; i < b_.size(); ++i) {
        while (b_[i] != nullptr) {
            Node* next = b_[i]->next;
            delete b_[i];
            b_[i] = next;
        }
    }
    n_ = 0;
}

// more buckets, every node relinked into its new bucket
template <typename K, typename V, typename H>
void HashMap<K, V, H>::rehash(std::size_t nb) {
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

} // namespace vas::ds
#endif
