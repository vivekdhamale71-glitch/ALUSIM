// Sorts a table of instruction names once with quick sort and finds each one with binary search.

#include <iostream>
#include <string>

struct Opcode {
    std::string mnemonic;
    std::string group;
    int operands;
};

const int TABLE_SIZE = 33;

// the table as it was written down, in no particular order
Opcode table[TABLE_SIZE] = {
    {"mov", "data transfer", 2}, {"add", "arithmetic", 2}, {"sub", "arithmetic", 2}, {"imul", "arithmetic", 2},
    {"idiv", "arithmetic", 1},   {"inc", "arithmetic", 1}, {"dec", "arithmetic", 1}, {"neg", "arithmetic", 1},
    {"cmp", "arithmetic", 2},    {"cqo", "arithmetic", 0}, {"and", "logic", 2},      {"or", "logic", 2},
    {"xor", "logic", 2},         {"not", "logic", 1},      {"shl", "logic", 2},      {"sal", "logic", 2},
    {"shr", "logic", 2},         {"test", "logic", 2},     {"jmp", "control flow", 1}, {"je", "control flow", 1},
    {"jz", "control flow", 1},   {"jne", "control flow", 1}, {"jnz", "control flow", 1}, {"jg", "control flow", 1},
    {"jge", "control flow", 1},  {"jl", "control flow", 1}, {"jle", "control flow", 1}, {"call", "control flow", 1},
    {"ret", "control flow", 0},  {"push", "stack", 1},     {"pop", "stack", 1},      {"nop", "system", 0},
    {"hlt", "system", 0}};

// ------------------------------------------------------------ quick sort by mnemonic
// moves entries smaller than the pivot to its left and larger ones to its right,
// then sorts both sides
void quickSortTable(int low, int high) {
    if (low >= high) {
        return;
    }
    std::string pivot = table[high].mnemonic;
    int boundary = low;
    for (int i = low; i < high; ++i) {
        if (table[i].mnemonic < pivot) {
            Opcode temp = table[i];
            table[i] = table[boundary];
            table[boundary] = temp;
            boundary = boundary + 1;
        }
    }
    Opcode temp = table[boundary];
    table[boundary] = table[high];
    table[high] = temp;
    quickSortTable(low, boundary - 1);
    quickSortTable(boundary + 1, high);
}

// ------------------------------------------------------------ binary search by mnemonic
// returns the index of the mnemonic, or -1; 'probes' counts the entries compared
int binaryLookup(const std::string& mnemonic, int& probes) {
    probes = 0;
    int low = 0;
    int high = TABLE_SIZE - 1;
    while (low <= high) {
        int middle = low + (high - low) / 2;
        probes = probes + 1;
        int order = mnemonic.compare(table[middle].mnemonic);
        if (order == 0) {
            return middle;
        }
        if (order > 0) {
            low = middle + 1;
        } else {
            high = middle - 1;
        }
    }
    return -1;
}

// for comparison: look at the entries one after the other
int linearLookup(const std::string& mnemonic, int& probes) {
    probes = 0;
    for (int i = 0; i < TABLE_SIZE; ++i) {
        probes = probes + 1;
        if (table[i].mnemonic == mnemonic) {
            return i;
        }
    }
    return -1;
}
