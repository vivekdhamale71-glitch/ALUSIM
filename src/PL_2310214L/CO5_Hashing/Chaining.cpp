#include "PL_2310214L/CO5_Hashing/Chaining.h"

namespace vas::ds {

bool ChainingTable::insert(const std::string& key, int value, ProbeTrace* t) {
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
    while (*pp) {                                  // walk the chain
        if ((*pp)->key == key) { (*pp)->value = value; tr.chainPos = pos; tr.found = true; return true; }
        pp = &(*pp)->next;
        ++pos;
    }
    *pp = new Node{key, value, nullptr};           // append at the end
    tr.chainPos = pos;
    ++count_;
    return true;
}

bool ChainingTable::find(const std::string& key, int& value, ProbeTrace* t) const {
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
            value = n->value;
            tr.finalIndex = tr.home;
            tr.chainPos = pos;
            tr.found = true;
            return true;
        }
    }
    return false;
}

std::vector<Bucket> ChainingTable::buckets() const {
    std::vector<Bucket> b(heads_.size());
    for (std::size_t i = 0; i < heads_.size(); ++i)
        for (Node* n = heads_[i]; n; n = n->next) b[i].push_back({n->key, n->value});
    return b;
}

void ChainingTable::freeAll() {
    for (auto& h : heads_) {
        while (h) { Node* n = h->next; delete h; h = n; }
    }
    count_ = 0;
}

// move every node to its new chain (keeps insertion order inside a chain)
void ChainingTable::rehash(int newCap) {
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

} // namespace vas::ds
