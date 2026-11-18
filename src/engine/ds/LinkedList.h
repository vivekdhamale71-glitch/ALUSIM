// =====================================================================
//  Linked lists                                              [PL CO2]
//  Each list type is used where its shape fits the job:
//   * SinglyLinkedList   - the token stream produced by the lexer
//   * DoublyLinkedList   - the execution timeline used by Step Back
//   * CircularLinkedList - the instruction-cycle ring
//                          FETCH -> DECODE -> EXECUTE -> MEMORY -> WB -> FETCH
// =====================================================================
#pragma once
#include "engine/ds/SinglyLinkedList.h"
#include "engine/ds/DoublyLinkedList.h"
#include "engine/ds/CircularLinkedList.h"
