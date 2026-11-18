// Backtracking that places N queens so that none attacks another.

#include <vector>

// Place N queens on an N x N board so that none attacks another. Backtracking: put a
// queen in the next row, and if no column works, go back and move the previous queen.
class NQueens {
public:
    explicit NQueens(int size) : size_(size), columnOfRow_(size, -1), solutions_(0) {
    }

    // counts every solution
    int countSolutions() {
        solutions_ = 0;
        placeRow(0);
        return solutions_;
    }

    // the columns of the first solution found, one per row (empty if there is none)
    std::vector<int> firstSolution() {
        firstFound_.clear();
        stopAtFirst_ = true;
        placeRow(0);
        stopAtFirst_ = false;
        return firstFound_;
    }

private:
    // is it safe to put a queen at (row, column)? No queen above may share the column or a diagonal.
    bool isSafe(int row, int column) const {
        for (int earlier = 0; earlier < row; ++earlier) {
            int other = columnOfRow_[earlier];
            if (other == column) {
                return false;
            }
            int rowDistance = row - earlier;
            int columnDistance = column - other;
            if (columnDistance < 0) {
                columnDistance = -columnDistance;
            }
            if (columnDistance == rowDistance) {
                return false;
            }
        }
        return true;
    }

    // tries every column of this row; each safe one is tried and then undone
    void placeRow(int row) {
        if (stopAtFirst_ && !firstFound_.empty()) {
            return;
        }
        if (row == size_) {
            solutions_ = solutions_ + 1;
            if (stopAtFirst_) {
                firstFound_ = columnOfRow_;
            }
            return;
        }
        for (int column = 0; column < size_; ++column) {
            if (isSafe(row, column)) {
                columnOfRow_[row] = column;           // place the queen
                placeRow(row + 1);
                columnOfRow_[row] = -1;               // take it back and try the next column
            }
        }
    }

    int size_;
    std::vector<int> columnOfRow_;
    std::vector<int> firstFound_;
    bool stopAtFirst_ = false;
    int solutions_;
};
