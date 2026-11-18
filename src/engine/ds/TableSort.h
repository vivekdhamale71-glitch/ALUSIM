// =====================================================================
//  quickSort<T>  -  generic quick sort                       [PL CO4]
//  Sorts the assembler's opcode table at start-up so that mnemonics
//  can then be found with binarySearch (TableSearch.h).
//  Defined in TableSort.cpp (included at the end: it is a template).
// =====================================================================
#pragma once
#include <vector>

namespace vas::ds {

// sorts a[lo..hi] in place; less(x, y) is "x comes before y"
template <typename T, typename Less>
void quickSort(std::vector<T>& a, Less less, int lo = 0, int hi = -2);

} // namespace vas::ds

#include "engine/ds/TableSort.cpp"
