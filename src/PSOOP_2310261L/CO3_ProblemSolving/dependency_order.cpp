// Builds the dependency graph of a block of instructions and puts them in an order that keeps every dependency.

#include <queue>
#include <string>
#include <vector>

// an instruction and the registers it reads and writes
struct Step {
    std::string text;
    std::vector<int> reads;
    std::vector<int> writes;
};

// an arrow of the graph: "later" must stay after "earlier"
struct Dependency {
    int earlier;
    int later;
    char kind;                 // 'R' read after write, 'W' write after read, 'O' write after write
};

// do two lists have a number in common?
static bool shareRegister(const std::vector<int>& a, const std::vector<int>& b) {
    for (std::size_t i = 0; i < a.size(); ++i) {
        for (std::size_t j = 0; j < b.size(); ++j) {
            if (a[i] == b[j]) {
                return true;
            }
        }
    }
    return false;
}

// builds the graph by comparing every pair of instructions:
//   later reads what earlier writes: a true dependency (RAW)
//   later writes what earlier reads: an anti-dependency (WAR)
//   later writes what earlier writes: an output dependency (WAW)
std::vector<Dependency> buildGraph(const std::vector<Step>& steps) {
    std::vector<Dependency> graph;
    for (int later = 1; later < static_cast<int>(steps.size()); ++later) {
        for (int earlier = 0; earlier < later; ++earlier) {
            if (shareRegister(steps[earlier].writes, steps[later].reads)) {
                Dependency d = {earlier, later, 'R'};
                graph.push_back(d);
            } else if (shareRegister(steps[earlier].reads, steps[later].writes)) {
                Dependency d = {earlier, later, 'W'};
                graph.push_back(d);
            } else if (shareRegister(steps[earlier].writes, steps[later].writes)) {
                Dependency d = {earlier, later, 'O'};
                graph.push_back(d);
            }
        }
    }
    return graph;
}

// topological order (Kahn's algorithm): repeatedly take an instruction that no remaining instruction must precede.
// A queue holds the ready ones; taking the smallest number first keeps the original order whenever it is allowed.
std::vector<int> topologicalOrder(int count, const std::vector<Dependency>& graph) {
    std::vector<int> waitingFor(count, 0);
    for (std::size_t i = 0; i < graph.size(); ++i) {
        waitingFor[graph[i].later] = waitingFor[graph[i].later] + 1;
    }
    std::queue<int> ready;
    for (int i = 0; i < count; ++i) {
        if (waitingFor[i] == 0) {
            ready.push(i);
        }
    }
    std::vector<int> order;
    while (!ready.empty()) {
        int current = ready.front();
        ready.pop();
        order.push_back(current);
        for (std::size_t i = 0; i < graph.size(); ++i) {
            if (graph[i].earlier == current) {
                waitingFor[graph[i].later] = waitingFor[graph[i].later] - 1;
                if (waitingFor[graph[i].later] == 0) {
                    ready.push(graph[i].later);
                }
            }
        }
    }
    return order;
}

// the length of the longest chain of true dependencies: no schedule can finish in fewer steps than this
int criticalPathLength(int count, const std::vector<Dependency>& graph) {
    std::vector<int> depth(count, 1);
    int longest = 0;
    for (int later = 0; later < count; ++later) {
        for (std::size_t i = 0; i < graph.size(); ++i) {
            if (graph[i].later == later && graph[i].kind == 'R' && depth[graph[i].earlier] + 1 > depth[later]) {
                depth[later] = depth[graph[i].earlier] + 1;
            }
        }
        if (depth[later] > longest) {
            longest = depth[later];
        }
    }
    return longest;
}

// two instructions may swap places when no arrow joins them
bool canSwap(const std::vector<Dependency>& graph, int first, int second) {
    for (std::size_t i = 0; i < graph.size(); ++i) {
        if ((graph[i].earlier == first && graph[i].later == second) || (graph[i].earlier == second && graph[i].later == first)) {
            return false;
        }
    }
    return true;
}
