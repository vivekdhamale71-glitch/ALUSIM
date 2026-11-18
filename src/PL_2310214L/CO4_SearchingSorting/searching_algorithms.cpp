// Linear, binary, jump, interpolation and Fibonacci search, each counting its probes.

#include <cmath>
#include <iostream>
#include <string>

// ------------------------------------------------------------ linear search
// looks at every element in turn: O(n), works on unsorted data
int linearSearch(const int a[], int n, int target, int& probes) {
    probes = 0;
    for (int i = 0; i < n; ++i) {
        probes = probes + 1;
        if (a[i] == target) {
            return i;
        }
    }
    return -1;
}

// ------------------------------------------------------------ binary search
// compares with the middle and throws away the half that cannot hold the target: O(log n)
int binarySearch(const int a[], int n, int target, int& probes) {
    probes = 0;
    int low = 0;
    int high = n - 1;
    while (low <= high) {
        int middle = low + (high - low) / 2;            // written this way so low + high cannot overflow
        probes = probes + 1;
        if (a[middle] == target) {
            return middle;
        }
        if (a[middle] < target) {
            low = middle + 1;
        } else {
            high = middle - 1;
        }
    }
    return -1;
}

// ------------------------------------------------------------ jump search
// jumps ahead in blocks of sqrt(n) until the block that may hold the target,
// then searches that block linearly: O(sqrt n)
int jumpSearch(const int a[], int n, int target, int& probes) {
    probes = 0;
    int block = static_cast<int>(std::sqrt(static_cast<double>(n)));
    if (block < 1) {
        block = 1;
    }
    int start = 0;
    while (start < n) {
        int last = start + block - 1;
        if (last > n - 1) {
            last = n - 1;
        }
        probes = probes + 1;
        if (a[last] >= target) {
            break;                                      // the target can only be inside this block
        }
        start = start + block;
    }
    if (start >= n) {
        return -1;
    }
    int end = start + block;
    if (end > n) {
        end = n;
    }
    for (int i = start; i < end; ++i) {
        probes = probes + 1;
        if (a[i] == target) {
            return i;
        }
    }
    return -1;
}

// ------------------------------------------------------------ interpolation search
// guesses where the target should be from its value (like looking for "Z" near the
// end of a dictionary): O(log log n) for evenly spread values
int interpolationSearch(const int a[], int n, int target, int& probes) {
    probes = 0;
    int low = 0;
    int high = n - 1;
    while (low <= high && target >= a[low] && target <= a[high]) {
        int position = low;
        if (a[high] != a[low]) {
            position = low + static_cast<int>((static_cast<long long>(target - a[low]) * (high - low)) / (a[high] - a[low]));
        }
        probes = probes + 1;
        if (a[position] == target) {
            return position;
        }
        if (a[position] < target) {
            low = position + 1;
        } else {
            high = position - 1;
        }
    }
    return -1;
}

// ------------------------------------------------------------ Fibonacci search
// like binary search but splits the range at Fibonacci numbers, so it needs only
// addition and subtraction, no division
int fibonacciSearch(const int a[], int n, int target, int& probes) {
    probes = 0;
    int fib2 = 0;               // F(k-2)
    int fib1 = 1;               // F(k-1)
    int fib = fib1 + fib2;      // F(k): the smallest Fibonacci number >= n
    while (fib < n) {
        fib2 = fib1;
        fib1 = fib;
        fib = fib1 + fib2;
    }
    int offset = -1;            // everything up to 'offset' has been ruled out
    while (fib > 1) {
        int i = offset + fib2;
        if (i > n - 1) {
            i = n - 1;
        }
        probes = probes + 1;
        if (a[i] < target) {                            // go right: step down one Fibonacci number
            fib = fib1;
            fib1 = fib2;
            fib2 = fib - fib1;
            offset = i;
        } else if (a[i] > target) {                     // go left: step down two Fibonacci numbers
            fib = fib2;
            fib1 = fib1 - fib2;
            fib2 = fib - fib1;
        } else {
            return i;
        }
    }
    if (fib1 == 1 && offset + 1 < n) {
        probes = probes + 1;
        if (a[offset + 1] == target) {
            return offset + 1;
        }
    }
    return -1;
}
