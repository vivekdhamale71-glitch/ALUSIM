// A circular queue used as a rolling buffer of recent samples and as the ready queue of Round Robin scheduling.

#include <iostream>
#include <string>

const int RING_SIZE = 5;

// a circular queue of integers with a fixed size
class RingQueue {
public:
    // overwrite = true: when full, the oldest item is dropped to make room
    explicit RingQueue(bool overwrite) : front_(0), count_(0), overwrite_(overwrite) {
    }

    // adds at the rear; returns false if the queue is full and may not overwrite
    bool enqueue(int value) {
        if (count_ == RING_SIZE) {
            if (!overwrite_) {
                return false;
            }
            front_ = (front_ + 1) % RING_SIZE;          // drop the oldest
            count_ = count_ - 1;
        }
        int rear = (front_ + count_) % RING_SIZE;       // wraps round the array
        items_[rear] = value;
        count_ = count_ + 1;
        return true;
    }

    // takes the oldest item from the front
    int dequeue() {
        int value = items_[front_];
        front_ = (front_ + 1) % RING_SIZE;
        count_ = count_ - 1;
        return value;
    }

    bool empty() const {
        return count_ == 0;
    }

    // oldest first
    std::string text() const {
        std::string result = "[";
        for (int i = 0; i < count_; ++i) {
            result = result + std::to_string(items_[(front_ + i) % RING_SIZE]);
            if (i + 1 < count_) {
                result = result + " ";
            }
        }
        return result + "]";
    }

private:
    int items_[RING_SIZE];
    int front_;
    int count_;
    bool overwrite_;
};

// ------------------------------------------------------------ Round Robin
struct Process {
    std::string name;
    int burst;          // CPU time it needs in total
    int remaining;      // CPU time it still needs
    int finish;         // time at which it completed
};

// runs the schedule; prints the Gantt chart and fills in every finish time
void roundRobin(Process processes[], int count, int quantum) {
    RingQueue ready(false);
    for (int i = 0; i < count; ++i) {
        processes[i].remaining = processes[i].burst;
        ready.enqueue(i);
    }
    int clock = 0;
    std::cout << "  ";
    while (!ready.empty()) {
        int index = ready.dequeue();                    // the process that waited longest
        int runFor = quantum;
        if (processes[index].remaining < runFor) {
            runFor = processes[index].remaining;
        }
        std::cout << "|" << processes[index].name << " " << clock << "-" << (clock + runFor) << " ";
        clock = clock + runFor;
        processes[index].remaining = processes[index].remaining - runFor;
        if (processes[index].remaining > 0) {
            ready.enqueue(index);                       // not finished: back of the line
        } else {
            processes[index].finish = clock;
        }
    }
    std::cout << "|\n";
}
