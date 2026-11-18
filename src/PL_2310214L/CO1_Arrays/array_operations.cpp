// Insert, delete, rotate, reverse and search in an array, counting how many elements each operation moves.

#include <iostream>
#include <string>

const int CAPACITY = 12;

class IntArray {
public:
    IntArray() : size_(0), moves_(0) {
    }

    // adds at the end: no shifting needed, O(1)
    bool append(int value) {
        if (size_ == CAPACITY) {
            return false;
        }
        data_[size_] = value;
        size_ = size_ + 1;
        return true;
    }

    // inserts in the middle: every element from 'position' on moves one place right
    bool insertAt(int position, int value) {
        if (size_ == CAPACITY || position < 0 || position > size_) {
            return false;
        }
        for (int i = size_; i > position; --i) {
            data_[i] = data_[i - 1];
            moves_ = moves_ + 1;
        }
        data_[position] = value;
        size_ = size_ + 1;
        return true;
    }

    // removes from the middle: every element after 'position' moves one place left
    bool removeAt(int position) {
        if (position < 0 || position >= size_) {
            return false;
        }
        for (int i = position; i < size_ - 1; ++i) {
            data_[i] = data_[i + 1];
            moves_ = moves_ + 1;
        }
        size_ = size_ - 1;
        return true;
    }

    // swaps the first and last element, then the second and second-to-last, ...
    void reverse() {
        int left = 0;
        int right = size_ - 1;
        while (left < right) {
            int temp = data_[left];
            data_[left] = data_[right];
            data_[right] = temp;
            left = left + 1;
            right = right - 1;
        }
    }

    // rotates left by one: the first element goes to the end
    void rotateLeftByOne() {
        if (size_ < 2) {
            return;
        }
        int first = data_[0];
        for (int i = 0; i < size_ - 1; ++i) {
            data_[i] = data_[i + 1];
            moves_ = moves_ + 1;
        }
        data_[size_ - 1] = first;
    }

    // position of the largest element (one pass)
    int indexOfMax() const {
        int best = 0;
        for (int i = 1; i < size_; ++i) {
            if (data_[i] > data_[best]) {
                best = i;
            }
        }
        return best;
    }

    // position of the first element equal to value, or -1
    int indexOf(int value) const {
        for (int i = 0; i < size_; ++i) {
            if (data_[i] == value) {
                return i;
            }
        }
        return -1;
    }

    int size() const {
        return size_;
    }

    int moves() const {
        return moves_;
    }

    void resetMoves() {
        moves_ = 0;
    }

    std::string text() const {
        std::string result = "[";
        for (int i = 0; i < size_; ++i) {
            result = result + std::to_string(data_[i]);
            if (i + 1 < size_) {
                result = result + ", ";
            }
        }
        return result + "]";
    }

private:
    int data_[CAPACITY];
    int size_;
    int moves_;
};
