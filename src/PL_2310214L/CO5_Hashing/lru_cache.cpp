// A hash table and a doubly linked list that keep the most recently used entries and drop the oldest.

#include <iostream>
#include <string>

// a node of the doubly linked list: one cached page
struct Node {
    std::string key;
    std::string content;
    Node* previous;
    Node* next;
};

// one cell of the hash table's chains: key -> list node
struct IndexEntry {
    std::string key;
    Node* node;
    IndexEntry* next;
};

const int BUCKETS = 11;

unsigned long long hashOf(const std::string& key) {
    unsigned long long h = 0;
    for (std::size_t i = 0; i < key.size(); ++i) {
        h = h * 31 + static_cast<unsigned char>(key[i]);
    }
    return h;
}

class LruCache {
public:
    explicit LruCache(int capacity) : capacity_(capacity), size_(0), head_(nullptr), tail_(nullptr), hits_(0), misses_(0) {
        for (int i = 0; i < BUCKETS; ++i) {
            buckets_[i] = nullptr;
        }
    }

    ~LruCache() {
        while (head_ != nullptr) {
            Node* following = head_->next;
            delete head_;
            head_ = following;
        }
        for (int i = 0; i < BUCKETS; ++i) {
            while (buckets_[i] != nullptr) {
                IndexEntry* following = buckets_[i]->next;
                delete buckets_[i];
                buckets_[i] = following;
            }
        }
    }

    // returns true and the content on a hit, and marks the entry as just used
    bool get(const std::string& key, std::string& content) {
        Node* node = findNode(key);
        if (node == nullptr) {
            misses_ = misses_ + 1;
            return false;
        }
        hits_ = hits_ + 1;
        moveToBack(node);
        content = node->content;
        return true;
    }

    // stores a page; if the cache is full the least recently used page is evicted
    void put(const std::string& key, const std::string& content) {
        Node* existing = findNode(key);
        if (existing != nullptr) {
            existing->content = content;
            moveToBack(existing);
            return;
        }
        if (size_ == capacity_) {
            Node* oldest = head_;                       // the front of the list is the oldest
            removeFromIndex(oldest->key);
            unlink(oldest);
            delete oldest;
            size_ = size_ - 1;
        }
        Node* node = new Node;
        node->key = key;
        node->content = content;
        node->previous = tail_;
        node->next = nullptr;
        if (tail_ != nullptr) {
            tail_->next = node;
        } else {
            head_ = node;
        }
        tail_ = node;
        addToIndex(key, node);
        size_ = size_ + 1;
    }

    int hits() const {
        return hits_;
    }

    int misses() const {
        return misses_;
    }

    // the keys, oldest first
    std::string order() const {
        std::string text = "[";
        for (Node* node = head_; node != nullptr; node = node->next) {
            text = text + node->key;
            if (node->next != nullptr) {
                text = text + " ";
            }
        }
        return text + "]";
    }

private:
    // --- the hash table part
    Node* findNode(const std::string& key) const {
        int bucket = static_cast<int>(hashOf(key) % BUCKETS);
        for (IndexEntry* e = buckets_[bucket]; e != nullptr; e = e->next) {
            if (e->key == key) {
                return e->node;
            }
        }
        return nullptr;
    }

    void addToIndex(const std::string& key, Node* node) {
        int bucket = static_cast<int>(hashOf(key) % BUCKETS);
        IndexEntry* entry = new IndexEntry;
        entry->key = key;
        entry->node = node;
        entry->next = buckets_[bucket];
        buckets_[bucket] = entry;
    }

    void removeFromIndex(const std::string& key) {
        int bucket = static_cast<int>(hashOf(key) % BUCKETS);
        IndexEntry* previous = nullptr;
        for (IndexEntry* e = buckets_[bucket]; e != nullptr; e = e->next) {
            if (e->key == key) {
                if (previous == nullptr) {
                    buckets_[bucket] = e->next;
                } else {
                    previous->next = e->next;
                }
                delete e;
                return;
            }
            previous = e;
        }
    }

    // --- the doubly linked list part
    // takes a node out of the list (its neighbours are joined) without deleting it
    void unlink(Node* node) {
        if (node->previous != nullptr) {
            node->previous->next = node->next;
        } else {
            head_ = node->next;
        }
        if (node->next != nullptr) {
            node->next->previous = node->previous;
        } else {
            tail_ = node->previous;
        }
    }

    // the node becomes the most recently used one
    void moveToBack(Node* node) {
        if (node == tail_) {
            return;
        }
        unlink(node);
        node->previous = tail_;
        node->next = nullptr;
        tail_->next = node;
        tail_ = node;
    }

    int capacity_;
    int size_;
    Node* head_;
    Node* tail_;
    IndexEntry* buckets_[BUCKETS];
    int hits_;
    int misses_;
};
