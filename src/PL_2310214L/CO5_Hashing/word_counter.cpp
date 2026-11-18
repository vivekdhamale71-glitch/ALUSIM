// Hash tables that count words, find duplicates and find two numbers with a given sum in one pass.

#include <iostream>
#include <string>
#include <vector>

const int BUCKETS = 101;

unsigned long long hashOf(const std::string& key) {
    unsigned long long h = 0;
    for (std::size_t i = 0; i < key.size(); ++i) {
        h = h * 31 + static_cast<unsigned char>(key[i]);
    }
    return h;
}

struct Cell {
    std::string key;
    int value;
    Cell* next;
};

// a hash table from text to a whole number, with separate chaining
class CountTable {
public:
    CountTable() {
        for (int i = 0; i < BUCKETS; ++i) {
            buckets_[i] = nullptr;
        }
    }

    ~CountTable() {
        for (int i = 0; i < BUCKETS; ++i) {
            while (buckets_[i] != nullptr) {
                Cell* following = buckets_[i]->next;
                delete buckets_[i];
                buckets_[i] = following;
            }
        }
    }

    // the stored value for a key, or nullptr if the key is new
    int* find(const std::string& key) {
        int bucket = static_cast<int>(hashOf(key) % BUCKETS);
        for (Cell* c = buckets_[bucket]; c != nullptr; c = c->next) {
            if (c->key == key) {
                return &c->value;
            }
        }
        return nullptr;
    }

    void put(const std::string& key, int value) {
        int* existing = find(key);
        if (existing != nullptr) {
            *existing = value;
            return;
        }
        int bucket = static_cast<int>(hashOf(key) % BUCKETS);
        Cell* cell = new Cell;
        cell->key = key;
        cell->value = value;
        cell->next = buckets_[bucket];
        buckets_[bucket] = cell;
    }

    // adds one to the count of a key (starting at 1 for a new key)
    void increment(const std::string& key) {
        int* count = find(key);
        if (count == nullptr) {
            put(key, 1);
        } else {
            *count = *count + 1;
        }
    }

private:
    Cell* buckets_[BUCKETS];
};

// ------------------------------------------------------------ 1. word frequencies
void countWords(const std::vector<std::string>& words, const char* wanted[], int wantedCount) {
    CountTable counts;
    for (std::size_t i = 0; i < words.size(); ++i) {
        counts.increment(words[i]);
    }
    for (int i = 0; i < wantedCount; ++i) {
        int* count = counts.find(wanted[i]);
        std::cout << "   \"" << wanted[i] << "\" appears ";
        if (count == nullptr) {
            std::cout << 0;
        } else {
            std::cout << *count;
        }
        std::cout << " times\n";
    }
}

// ------------------------------------------------------------ 2. duplicates
// brute force: compare every pair
long duplicatesBruteForce(const std::vector<int>& numbers, int& repeats) {
    long steps = 0;
    repeats = 0;
    for (std::size_t i = 0; i < numbers.size(); ++i) {
        for (std::size_t j = 0; j < i; ++j) {
            steps = steps + 1;
            if (numbers[i] == numbers[j]) {
                repeats = repeats + 1;
                break;
            }
        }
    }
    return steps;
}

// hashing: one pass, a table remembers which numbers were seen
long duplicatesWithHashing(const std::vector<int>& numbers, int& repeats) {
    CountTable seen;
    long steps = 0;
    repeats = 0;
    for (std::size_t i = 0; i < numbers.size(); ++i) {
        steps = steps + 1;
        std::string key = std::to_string(numbers[i]);
        if (seen.find(key) != nullptr) {
            repeats = repeats + 1;
        } else {
            seen.put(key, 1);
        }
    }
    return steps;
}

// ------------------------------------------------------------ 3. two numbers with a given sum
// true if numbers[first] + numbers[second] == target; 'lookups' counts the table lookups
bool twoSum(const std::vector<int>& numbers, int target, int& first, int& second, long& lookups) {
    CountTable seenAt;                                  // number -> position where it was seen
    lookups = 0;
    for (std::size_t i = 0; i < numbers.size(); ++i) {
        int needed = target - numbers[i];               // the partner this number would need
        lookups = lookups + 1;
        int* position = seenAt.find(std::to_string(needed));
        if (position != nullptr) {
            first = *position;
            second = static_cast<int>(i);
            return true;
        }
        seenAt.put(std::to_string(numbers[i]), static_cast<int>(i));
    }
    return false;
}
