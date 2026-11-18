// Bubble, insertion, selection, shell, merge and quick sort, each counting its comparisons and moves.

#include <iostream>
#include <string>
#include <vector>

// the work an algorithm did
struct Counts {
    long comparisons;
    long moves;
};

// ------------------------------------------------------------ bubble sort
// swaps neighbours that are out of order; after each pass the largest value
// has "bubbled" to the end; stops early if a pass makes no swap
void bubbleSort(int a[], int n, Counts& c) {
    for (int pass = 0; pass < n - 1; ++pass) {
        bool swapped = false;
        for (int i = 0; i < n - 1 - pass; ++i) {
            c.comparisons = c.comparisons + 1;
            if (a[i] > a[i + 1]) {
                int temp = a[i];
                a[i] = a[i + 1];
                a[i + 1] = temp;
                c.moves = c.moves + 1;
                swapped = true;
            }
        }
        if (!swapped) {
            return;
        }
    }
}

// ------------------------------------------------------------ insertion sort
// grows a sorted prefix: each new value slides left until it is in place
void insertionSort(int a[], int n, Counts& c) {
    for (int i = 1; i < n; ++i) {
        int value = a[i];
        int j = i - 1;
        while (j >= 0) {
            c.comparisons = c.comparisons + 1;
            if (a[j] <= value) {
                break;
            }
            a[j + 1] = a[j];
            c.moves = c.moves + 1;
            j = j - 1;
        }
        a[j + 1] = value;
    }
}

// ------------------------------------------------------------ selection sort
// finds the smallest of the rest and swaps it to the front: few moves, many compares
void selectionSort(int a[], int n, Counts& c) {
    for (int i = 0; i < n - 1; ++i) {
        int smallest = i;
        for (int j = i + 1; j < n; ++j) {
            c.comparisons = c.comparisons + 1;
            if (a[j] < a[smallest]) {
                smallest = j;
            }
        }
        if (smallest != i) {
            int temp = a[i];
            a[i] = a[smallest];
            a[smallest] = temp;
            c.moves = c.moves + 1;
        }
    }
}

// ------------------------------------------------------------ shell sort
// insertion sort on elements that are 'gap' apart, with a shrinking gap, so that
// far-away values travel quickly
void shellSort(int a[], int n, Counts& c) {
    for (int gap = n / 2; gap > 0; gap = gap / 2) {
        for (int i = gap; i < n; ++i) {
            int value = a[i];
            int j = i;
            while (j >= gap) {
                c.comparisons = c.comparisons + 1;
                if (a[j - gap] <= value) {
                    break;
                }
                a[j] = a[j - gap];
                c.moves = c.moves + 1;
                j = j - gap;
            }
            a[j] = value;
        }
    }
}

// ------------------------------------------------------------ merge sort
// splits the array in two, sorts each half, then merges the two sorted halves
void mergeParts(int a[], int low, int middle, int high, Counts& c) {
    std::vector<int> merged;
    int left = low;
    int right = middle + 1;
    while (left <= middle && right <= high) {
        c.comparisons = c.comparisons + 1;
        if (a[left] <= a[right]) {
            merged.push_back(a[left]);
            left = left + 1;
        } else {
            merged.push_back(a[right]);
            right = right + 1;
        }
    }
    while (left <= middle) {
        merged.push_back(a[left]);
        left = left + 1;
    }
    while (right <= high) {
        merged.push_back(a[right]);
        right = right + 1;
    }
    for (std::size_t i = 0; i < merged.size(); ++i) {
        a[low + static_cast<int>(i)] = merged[i];
        c.moves = c.moves + 1;
    }
}

void mergeSortRange(int a[], int low, int high, Counts& c) {
    if (low >= high) {
        return;
    }
    int middle = (low + high) / 2;
    mergeSortRange(a, low, middle, c);
    mergeSortRange(a, middle + 1, high, c);
    mergeParts(a, low, middle, high, c);
}

void mergeSort(int a[], int n, Counts& c) {
    mergeSortRange(a, 0, n - 1, c);
}

// ------------------------------------------------------------ quick sort
// picks the last element as the pivot, moves smaller values before it and larger
// after it, then sorts both sides
void quickSortRange(int a[], int low, int high, Counts& c) {
    if (low >= high) {
        return;
    }
    int pivot = a[high];
    int boundary = low;                         // a[low .. boundary-1] are smaller than the pivot
    for (int i = low; i < high; ++i) {
        c.comparisons = c.comparisons + 1;
        if (a[i] < pivot) {
            int temp = a[i];
            a[i] = a[boundary];
            a[boundary] = temp;
            c.moves = c.moves + 1;
            boundary = boundary + 1;
        }
    }
    int temp = a[boundary];
    a[boundary] = a[high];
    a[high] = temp;
    c.moves = c.moves + 1;
    quickSortRange(a, low, boundary - 1, c);
    quickSortRange(a, boundary + 1, high, c);
}

void quickSort(int a[], int n, Counts& c) {
    quickSortRange(a, 0, n - 1, c);
}

// ------------------------------------------------------------ the experiment

// the same "random" numbers every run
