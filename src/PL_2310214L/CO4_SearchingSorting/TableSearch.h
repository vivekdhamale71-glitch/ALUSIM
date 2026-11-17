// =====================================================================
//  binarySearch<T>  -  generic binary search                 [PL CO4]
//  Used by the program itself on sorted tables:
//    * the lexer looks up keywords (if / else / while ...)
//    * the assembler looks up mnemonics in the opcode table, which was
//      sorted with quickSort (TableSort.h) at start-up
//  Defined in TableSearch.cpp (included at the end: it is a template).
// =====================================================================
#pragma once
#include <vector>

namespace vas::ds {

// cmp(element, key) returns <0, 0, >0. `probes` collects visited indices.
// Returns the index, or -1 if the key is not in the table.
template <typename T, typename K, typename Cmp>
int binarySearch(const std::vector<T>& a, const K& key, Cmp cmp, std::vector<int>* probes = nullptr);

} // namespace vas::ds

#include "PL_2310214L/CO4_SearchingSorting/TableSearch.cpp"
