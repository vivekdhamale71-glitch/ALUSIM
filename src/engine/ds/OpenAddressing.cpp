#include "engine/ds/OpenAddressing.h"

namespace vas::ds {

int LinearProbingTable::probeIndex(int home, int i) const {
    return (home + i) % capacity();
}

int QuadraticProbingTable::probeIndex(int home, int i) const {
    return int((static_cast<long long>(home) + static_cast<long long>(i) * i) % capacity());
}

bool OpenAddressingTable::insert(const std::string& key, int value, ProbeTrace* t) {
    ProbeTrace local;
    ProbeTrace* chosen = t;
    if (chosen == nullptr) {
        chosen = &local;
    }
    ProbeTrace& tr = *chosen;
    tr = ProbeTrace{};
    tr.key = key;
    // keep the load factor below 0.7 so probe sequences stay short
    if ((count_ + 1) > 0.7 * capacity()) {
        tr.rehashed = true;
        tr.oldCapacity = capacity();
        rehash(nextPrime(capacity() * 2 + 1));
    }
    while (true) {
        if (place(key, value, tr)) {
            return true;
        }
        tr.rehashed = true;                  // quadratic probing found no free slot
        tr.oldCapacity = capacity();
        rehash(nextPrime(capacity() * 2 + 1));
    }
}

// follow the same probe sequence as insert; an empty slot means "absent"
bool OpenAddressingTable::find(const std::string& key, int& value, ProbeTrace* t) const {
    ProbeTrace local;
    ProbeTrace* chosen = t;
    if (chosen == nullptr) {
        chosen = &local;
    }
    ProbeTrace& tr = *chosen;
    tr = ProbeTrace{};
    tr.key = key;
    tr.rawHash = polyHash(key);
    tr.capacity = capacity();
    tr.home = int(tr.rawHash % capacity());
    for (int i = 0; i < capacity(); ++i) {
        int idx = probeIndex(tr.home, i);
        tr.probes.push_back(idx);
        ++probes_;
        if (!slots_[idx].used) {
            return false;
        }
        if (slots_[idx].key == key) {
            value = slots_[idx].value;
            tr.finalIndex = idx;
            tr.found = true;
            return true;
        }
    }
    return false;
}

void OpenAddressingTable::clear() {
    for (std::size_t i = 0; i < slots_.size(); ++i) {
        slots_[i] = Slot{};
    }
    count_ = 0;
    probes_ = collisions_ = 0;
}

std::vector<Bucket> OpenAddressingTable::buckets() const {
    std::vector<Bucket> b(slots_.size());
    for (std::size_t i = 0; i < slots_.size(); ++i) {
        if (slots_[i].used) {
            b[i].push_back({slots_[i].key, slots_[i].value});
        }
    }
    return b;
}

// probe from the home slot until the key or an empty slot is found
bool OpenAddressingTable::place(const std::string& key, int value, ProbeTrace& tr) {
    tr.probes.clear();
    tr.rawHash = polyHash(key);
    tr.capacity = capacity();
    tr.home = int(tr.rawHash % capacity());
    for (int i = 0; i < capacity(); ++i) {
        int idx = probeIndex(tr.home, i);
        tr.probes.push_back(idx);
        ++probes_;
        Slot& s = slots_[idx];
        if (s.used && s.key == key) {            // update an existing key
            s.value = value;
            tr.finalIndex = idx;
            tr.found = true;
            return true;
        }
        if (!s.used) {
            s.key = key;
            s.value = value;
            s.used = true;
            ++count_;
            tr.finalIndex = idx;
            return true;
        }
        if (i == 0) {
            tr.collision = true;
            ++collisions_;
        }
    }
    return false;
}

// new prime-sized array, every key re-inserted (its home slot changes)
void OpenAddressingTable::rehash(int newCap) {
    std::vector<Slot> old = std::move(slots_);
    slots_.assign(newCap, Slot{});
    count_ = 0;
    int p = probes_, c = collisions_;
    ProbeTrace dummy;
    for (std::size_t i = 0; i < old.size(); ++i) {
        if (old[i].used) {
            place(old[i].key, old[i].value, dummy);
        }
    }
    probes_ = p;                             // rehash work is not counted as user probes
    collisions_ = c;
}

} // namespace vas::ds
