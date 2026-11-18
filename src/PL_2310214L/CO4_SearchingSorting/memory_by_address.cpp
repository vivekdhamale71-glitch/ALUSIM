// Sorts memory words by address and names each address with a binary search.

#include <iostream>
#include <string>

struct MemoryWord {
    long long address;
    long long value;
    std::string name;
};

struct Variable {
    std::string name;
    long long address;
};

const long long STACK_LIMIT = 0x7000;

// moves one entry left until the one before it is not larger; keeps a growing sorted front part
void sortWordsByAddress(MemoryWord words[], int count) {
    for (int i = 1; i < count; ++i) {
        MemoryWord current = words[i];
        int j = i - 1;
        while (j >= 0 && words[j].address > current.address) {
            words[j + 1] = words[j];
            j = j - 1;
        }
        words[j + 1] = current;
    }
}

void sortVariablesByAddress(Variable variables[], int count) {
    for (int i = 1; i < count; ++i) {
        Variable current = variables[i];
        int j = i - 1;
        while (j >= 0 && variables[j].address > current.address) {
            variables[j + 1] = variables[j];
            j = j - 1;
        }
        variables[j + 1] = current;
    }
}

// binary search for the variable that lives at an address; -1 if none does
int findVariable(const Variable variables[], int count, long long address) {
    int low = 0;
    int high = count - 1;
    while (low <= high) {
        int middle = low + (high - low) / 2;
        if (variables[middle].address == address) {
            return middle;
        }
        if (variables[middle].address < address) {
            low = middle + 1;
        } else {
            high = middle - 1;
        }
    }
    return -1;
}

// gives each word its name: a variable's name, "stack" above the stack limit, or "-"
void nameTheWords(MemoryWord words[], int count, const Variable variables[], int variableCount) {
    for (int i = 0; i < count; ++i) {
        int index = findVariable(variables, variableCount, words[i].address);
        if (index >= 0) {
            words[i].name = variables[index].name;
        } else if (words[i].address >= STACK_LIMIT) {
            words[i].name = "stack";
        } else {
            words[i].name = "-";
        }
    }
}
