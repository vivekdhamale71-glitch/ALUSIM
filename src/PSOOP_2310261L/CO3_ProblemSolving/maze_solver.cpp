// A breadth-first search with a queue that finds the shortest way through a maze.

#include <queue>
#include <string>
#include <vector>

struct Position {
    int row;
    int column;
};

// A maze drawn with text: '#' is a wall, '.' is free, 'S' is the start and 'E' the exit.
class Maze {
public:
    explicit Maze(const std::vector<std::string>& rows) : rows_(rows) {
    }

    // The shortest way from S to E, or an empty list if there is none.
    // Breadth-first search: visit every cell one step away, then two steps away, and so on,
    // so the first time the exit is reached the way there is the shortest one.
    std::vector<Position> shortestPath() const {
        int height = static_cast<int>(rows_.size());
        int width = static_cast<int>(rows_[0].size());
        std::vector<std::vector<int> > cameFrom(height, std::vector<int>(width, -2));   // -2 = not visited
        Position start = find('S');
        Position exit = find('E');

        std::queue<Position> waiting;
        waiting.push(start);
        cameFrom[start.row][start.column] = -1;           // the start has no previous cell
        const int stepRow[4] = {-1, 1, 0, 0};
        const int stepColumn[4] = {0, 0, -1, 1};

        while (!waiting.empty()) {
            Position current = waiting.front();
            waiting.pop();
            if (current.row == exit.row && current.column == exit.column) {
                return walkBack(cameFrom, exit, width);
            }
            for (int direction = 0; direction < 4; ++direction) {
                int row = current.row + stepRow[direction];
                int column = current.column + stepColumn[direction];
                if (row < 0 || row >= height || column < 0 || column >= width) {
                    continue;
                }
                if (rows_[row][column] == '#' || cameFrom[row][column] != -2) {
                    continue;
                }
                cameFrom[row][column] = current.row * width + current.column;   // remember where we came from
                Position next;
                next.row = row;
                next.column = column;
                waiting.push(next);
            }
        }
        return std::vector<Position>();
    }

private:
    Position find(char symbol) const {
        Position position;
        position.row = 0;
        position.column = 0;
        for (std::size_t r = 0; r < rows_.size(); ++r) {
            for (std::size_t c = 0; c < rows_[r].size(); ++c) {
                if (rows_[r][c] == symbol) {
                    position.row = static_cast<int>(r);
                    position.column = static_cast<int>(c);
                }
            }
        }
        return position;
    }

    // follows the "came from" marks from the exit back to the start, then reverses the list
    std::vector<Position> walkBack(const std::vector<std::vector<int> >& cameFrom, Position exit, int width) const {
        std::vector<Position> path;
        Position current = exit;
        while (true) {
            path.insert(path.begin(), current);
            int previous = cameFrom[current.row][current.column];
            if (previous < 0) {
                break;
            }
            current.row = previous / width;
            current.column = previous % width;
        }
        return path;
    }

    std::vector<std::string> rows_;
};
