"""In-depth explanation text for the five Programming Languages course outcomes.

Same block format as cg_deep_content.py. Every statement describes code that exists in
src/engine/ds, src/engine/lang, src/core or src/app; the worked numbers were calculated, not guessed.
"""

PL = [
(1, "CO1_Arrays", "Apply suitable data structures such as arrays for solving given problems.", [
 ("The idea", [
  ("p", "An array keeps its elements one after the other in memory, all of the same size. Element i therefore lives at  start + i x size, so the computer can jump straight to any element without looking at the others. "
        "That makes an array the right structure whenever the number of items is fixed and they are known by position.")]),
 ("The register file is an array", [
  ("p", "The simulated processor has 16 general registers (rax, rcx, rdx, rbx, rsp, rbp, rsi, rdi, r8 to r15), each 64 bits wide. They are kept in one FixedArray<Register64, 16> inside the CPU class (core/CPU.h). "
        "A register is chosen by its number, so index 0 is rax and index 15 is r15; the assembler turns the name into the number once and the CPU then uses the number."),
  ("table", ["Operation", "How it works", "Cost"], [
    ["operator[] (fast access)", "returns data_[i] directly with no check; used inside the CPU where the index always comes from a decoded instruction", "O(1)"],
    ["at() (checked access)", "if i >= N it throws std::out_of_range with the bad index in the message; otherwise returns data_[i]", "O(1)"],
    ["fill(v)", "visits every element once and stores v (used to clear all registers)", "O(N)"],
    ["size()", "a constant: N is a template parameter, so it is known when the program is compiled", "O(1)"]]),
  ("p", "Why an array and not a list: the register number is the index, a register file never grows or shrinks, and every access in every executed instruction is by number. A list would need to walk to the register, an array does not. "
        "The storage is contiguous: 16 registers x 8 bytes = 128 bytes in one block.")]),
 ("Safe and fast access, side by side", [
  ("steps", [
    "Fast path: regs_[3] is compiled to one memory read at start + 3 x 8. Nothing is tested, so a wrong index would silently read someone else's memory.",
    "Safe path: regs_.at(20) compares 20 with N = 16 first and throws, so a bug is reported instead of corrupting memory.",
    "The program uses the fast path inside the CPU, where the index is always valid, and the safe path for anything that comes from outside (for example a register number typed by the user)."]),
  ("p", "The same idea is written without a template in the CO folder (register_file.cpp): a RegisterFile class holds the 16 values with read, write, checkedRead and the 32, 16 and 8-bit views of one register, which is what the sub-register panel of the interface shows for rax (eax, ax, ah and al).")]),
 ("Other arrays in the program", [
  ("p", "The Data Lab view keeps the values it sorts and searches in a std::vector<int>: a growable array with the same contiguous layout. The sorting and searching algorithms of CO4 work on exactly these arrays by index, which is why array access being O(1) matters to them.")]),
 ("Where it is implemented", [
  ("table", ["File", "What is in it"], [
    ["engine/ds/FixedArray.h, FixedArray.cpp", "The FixedArray<T, N> template: operator[], at(), fill(), size() and begin() / end()."],
    ["core/CPU.h", "regs_, the register file: FixedArray<Register64, REG_COUNT>."],
    ["CO1_Arrays/register_file.cpp", "A register file as a fixed array with indexed and checked access and the sub-register views."],
    ["CO1_Arrays/array_operations.cpp", "The basic array operations: traverse, insert, delete, search, reverse and rotate."],
    ["CO1_Arrays/grid_tables.cpp", "Two-dimensional arrays stored row by row, with the index calculation."]])]),
]),

(2, "CO2_LinkedLists", "Apply linked lists to develop real world applications.", [
 ("The idea", [
  ("p", "A linked list is a chain of nodes. Each node holds a value and a pointer to the next node, so the nodes can be anywhere in memory and the list can grow one node at a time. "
        "Inserting at a known place is cheap because nothing has to be moved, but reaching element k means walking k nodes. The program uses three kinds, each where its shape fits the job.")]),
 ("Singly linked list: the token stream", [
  ("p", "The lexer cuts the source text into tokens (numbers, names, operators) and appends each one to a SinglyLinkedList<Token>. The parser then walks the list once from the front to the back."),
  ("steps", [
    "pushBack(v) makes a new node. If the list is empty, head and tail both point to it.",
    "Otherwise the old tail's next pointer is set to the new node, and the tail pointer moves to it. Because a tail pointer is kept, this is O(1) and never walks the list.",
    "An Iterator holds one node pointer. ++ moves it to node->next, * reads the value, and the end is the null pointer, so a for loop over the list needs no index.",
    "clear() deletes the nodes from the front, saving next before each delete so the chain is not lost."]),
  ("p", "Why a list: the number of tokens is not known until the whole text is read, tokens are only ever added at the end, and they are read once in order. That is exactly what a singly linked list does well.")]),
 ("Doubly linked list: the execution history", [
  ("p", "Before every instruction runs, the simulator saves a snapshot of the CPU (registers, flags, memory) into a DoublyLinkedList. Each node has both a prev and a next pointer, so the list can be walked backwards from the tail."),
  ("steps", [
    "Run one instruction: pushBack(entry) links a new node after the tail (new->prev = tail, tail->next = new).",
    "Step Back: take the tail node and restore the CPU from the snapshot saved in it.",
    "Then truncateAfter(tail->prev) deletes the nodes after that point; with the tail removed, the node before it becomes the new tail.",
    "Stepping back again repeats this, one instruction per press, until the list is empty (the program then says it is already at the first instruction)."]),
  ("p", "Why doubly linked: undo needs to go from the newest node to the one before it without scanning from the front. With only next pointers that would cost O(n) for every step back; with prev it is O(1). The list owns its snapshots, so its copy operations are deleted.")]),
 ("Circular linked list: the instruction cycle", [
  ("p", "A processor repeats FETCH, DECODE, EXECUTE, MEMORY, WRITE-BACK forever. The program stores these five phases in a CircularLinkedList<Phase>: the last node points back to the first."),
  ("steps", [
    "insert(v) puts the new node between the tail and the head and makes it the new tail. A list of one node points to itself.",
    "Only the tail pointer is kept. The head is tail->next, so the ring needs one pointer instead of two.",
    "The phase after X is find(X)->next. There is no special case for the end: after WRITE-BACK the next node is FETCH again.",
    "find(v) walks at most one lap (size steps) and returns null if the value is not in the ring, so it can never loop forever."]),
  ("p", "The datapath view uses this ring to know which phase comes next while it animates the signals, and to draw the five phases in a loop.")]),
 ("The three compared", [
  ("table", ["List", "Insert at the end", "Go backwards", "Used for"], [
    ["Singly", "O(1) with a tail pointer", "no", "tokens, read once front to back"],
    ["Doubly", "O(1)", "O(1) per step", "undo history (Step Back)"],
    ["Circular", "O(1)", "no", "the repeating five-phase cycle"]])]),
 ("Where it is implemented", [
  ("table", ["File", "What is in it"], [
    ["engine/ds/SinglyLinkedList.h, .cpp", "pushBack, clear, front and the Iterator."],
    ["engine/ds/DoublyLinkedList.h, .cpp", "pushBack and truncateAfter (clear() is truncateAfter(null))."],
    ["engine/ds/CircularLinkedList.h, .cpp", "insert, find and clear for the ring."],
    ["engine/lang/Compiler.cpp", "The token list: the lexer appends with pushBack and the parser walks it with the iterator."],
    ["app/Sim.cpp, app/Sim.h", "history (the doubly linked list), pushBack after each instruction and stepBack with truncateAfter; phaseRing."],
    ["app/DatapathView.cpp", "Walks the phase ring with head() and find(phase)->next."],
    ["CO2_LinkedLists/singly_linked_list.cpp, doubly_linked_list.cpp, circular_linked_list.cpp", "The three lists written out with insert, delete, search and traverse."]])]),
]),

(3, "CO3_StackQueue", "Apply stack and queue to solve real world problems.", [
 ("The idea", [
  ("p", "A stack gives items back in the opposite order they were put in (last in, first out). A queue gives them back in the order they arrived (first in, first out). "
        "Both are built by hand here on arrays, and the compiler uses them together to turn an expression such as 2 + 3 * 4 into machine instructions.")]),
 ("Stack on a growing array", [
  ("steps", [
    "The stack keeps an array, a size and a capacity. The top element is data_[size - 1].",
    "push(v): if the array is full, grow() first (capacity 8 at the start, then doubled each time and all elements moved across). Then store v at data_[size] and add one to the size.",
    "pop(): if the stack is empty it throws std::underflow_error; otherwise it subtracts one from the size and returns that element.",
    "top() returns the top element without removing it (the peek), with the same empty check."]),
  ("p", "Doubling the array makes push O(1) on average: copying n items happens only once every n pushes, so the cost spread over the pushes is a constant. pop, top and size are O(1) always.")]),
 ("Circular queue on a ring buffer", [
  ("steps", [
    "The queue keeps an array, its capacity, head (the index of the front) and count.",
    "enqueue(v): the rear is at index (head + count) mod capacity. Store v there and add one to the count. The index wraps round to 0 at the end of the array, so freed space at the front is reused.",
    "dequeue(): if empty, throw std::underflow_error. Otherwise take data_[head], set head = (head + 1) mod capacity and subtract one from the count.",
    "When the queue is full, enqueue either grows (doubles the capacity and copies the elements in order so that the front is at index 0 again) or, in overwrite mode, drops the oldest element."]),
  ("p", "Both enqueue and dequeue are O(1). Wrapping with mod instead of moving elements is what avoids the O(n) shift that a plain array queue would need.")]),
 ("Where they are used", [
  ("table", ["Use", "Structure", "What it does in the program"], [
    ["Operator stack", "Stack<std::string>", "The shunting-yard algorithm holds operators here until their operands are done."],
    ["Output queue", "CircularQueue<PostItem>, capacity 16", "Operands and operators leave the shunting-yard in postfix order through this queue."],
    ["Value stack", "Stack<Value>", "Code generation reads the postfix list; operands are pushed and every operator pops its operands."],
    ["Free registers", "Stack<int>", "The compiler pushes every register it may use. It pops one to hold a value and pushes it back when the value is no longer needed."],
    ["Waveform history", "CircularQueue<SignalSample>, capacity 56, overwrite", "The last 56 control-signal samples. When it is full, each new sample drops the oldest, so the waveform scrolls."]])]),
 ("Shunting-yard worked out: 2 + 3 * 4", [
  ("p", "The rule: a number goes straight to the output queue. An operator first pops every operator on the stack that has higher precedence (or equal, for a left-associative one) to the output, then is pushed itself. At the end every operator left on the stack is popped to the output."),
  ("table", ["Token", "Operator stack", "Output queue", "What happened"], [
    ["2", "(empty)", "2", "number: straight to the output"],
    ["+", "+", "2", "stack was empty: push"],
    ["3", "+", "2 3", "number: straight to the output"],
    ["*", "+ *", "2 3", "* binds tighter than +, so nothing is popped; push"],
    ["4", "+ *", "2 3 4", "number: straight to the output"],
    ["end", "(empty)", "2 3 4 * +", "pop * then pop + to the output"]]),
  ("p", "The program records a snapshot of both structures after every action, which is what the Parse tree tab shows as the stack on the left. A wrong expression is reported by the checks in the loop, for example a missing operator or a missing value.")]),
 ("Evaluating the postfix with the value stack", [
  ("table", ["Item", "Value stack (bottom to top)", "What happened"], [
    ["2", "2", "push"],
    ["3", "2 3", "push"],
    ["4", "2 3 4", "push"],
    ["*", "2 12", "pop 4 and 3, multiply, push 12"],
    ["+", "14", "pop 12 and 2, add, push 14"]]),
  ("p", "Code generation does the same, but instead of numbers it pushes where each value lives (an immediate or a register), and every operator emits one assembly instruction. That is why a stack of free registers is needed next to it.")]),
 ("Where it is implemented", [
  ("table", ["File", "What is in it"], [
    ["engine/ds/Stack.h, Stack.cpp", "push, pop, top, grow, copy and swap."],
    ["engine/ds/Queue.h, Queue.cpp", "CircularQueue: enqueue, dequeue, front, grow and the overwrite mode."],
    ["engine/lang/Compiler.cpp", "shunt() (operator stack and output queue), genPostfix() (value stack) and the free register stack."],
    ["app/Sim.h, app/Sim.cpp", "The waveform history queue, enqueue once per cycle."],
    ["CO3_StackQueue/bracket_checker.cpp", "Checks that brackets are balanced, with a stack."],
    ["CO3_StackQueue/postfix_calculator.cpp", "Evaluates a postfix expression with a stack."],
    ["CO3_StackQueue/undo_redo_history.cpp", "Undo and redo with two stacks."],
    ["CO3_StackQueue/round_robin_queue.cpp", "A round-robin process queue."]])]),
]),

(4, "CO4_SearchingSorting", "Apply searching and sorting techniques to solve real world problems.", [
 ("The idea", [
  ("p", "Sorting puts items in order; searching finds one. The cost of both is counted in comparisons. A search on sorted data can throw half the data away at every step, so it needs only about log2(n) comparisons instead of n. "
        "The Data Lab view records every comparison and swap and replays them, so the algorithms can be watched.")]),
 ("How the Data Lab records an algorithm", [
  ("p", "Each sorter works on a copy of the array and writes a list of steps (SortStep): Compare(i, j), Swap(i, j), Pivot(i), Sorted(i) and Done. Each searcher writes a list of SearchStep (the index probed, the current low and high, and whether it was found). "
        "The screen then plays the list back one step at a time, which is why the same animation can be paused or stepped. The number of steps is the comparison count shown in the RESULT card.")]),
 ("Sorting", [
  ("table", ["Algorithm", "How it works", "Cost"], [
    ["Bubble", "Each pass compares neighbours j and j+1 and swaps them if they are out of order, so the largest remaining value is carried to the end. If a whole pass makes no swap, the data is sorted and it stops early.", "O(n^2) compares, stable"],
    ["Insertion", "Keeps a sorted part at the front. The next value moves left past every larger neighbour (compare, swap, repeat) until its left neighbour is not larger.", "O(n^2) worst case, fast on nearly sorted data, stable"],
    ["Quick", "Takes the last element as the pivot, moves every smaller element to the front part (Lomuto partition), puts the pivot between the two parts (now in its final place), then sorts both parts.", "O(n log n) on average"]]),
  ("p", "Worked example, [5, 2, 4, 1]: Bubble sort makes 6 comparisons and 5 swaps; Insertion sort also makes 6 comparisons and 5 swaps. Both end with [1, 2, 4, 5]. "
        "The count of swaps equals the number of pairs that were in the wrong order (here 5), and neither algorithm can do fewer swaps than that because each swap fixes only one such pair.")]),
 ("Searching", [
  ("table", ["Algorithm", "How it works", "Cost"], [
    ["Linear", "Compares every element from the first one until the target is found. Works on unsorted data.", "O(n)"],
    ["Binary", "Looks at the middle element of the range [lo, hi]. If it is the target, done; if it is smaller, the target can only be on the right (lo = mid + 1); otherwise on the left (hi = mid - 1). The middle is lo + (hi - lo) / 2, so lo + hi can never overflow. Needs sorted data.", "O(log n)"],
    ["Fibonacci", "Splits the range with Fibonacci numbers instead of halves, using only addition and subtraction. It probes at offset + F(k-2); a smaller value moves right by one Fibonacci step, a larger value moves left by two. Needs sorted data.", "O(log n)"]]),
  ("p", "Worked example: find 26 in the sorted array [3, 8, 12, 17, 21, 26, 34]. Linear search compares 3, 8, 12, 17, 21, 26: 6 comparisons. Binary search probes index 3 (17, too small, so lo = 4) then index 5 (26, found): 2 comparisons. Fibonacci search probes indexes 2, 4 and 5: 3 comparisons. "
        "The program's \"all three for this target\" panel shows exactly this kind of comparison for the chosen cell. A search that needs sorted data is given a sorted copy (made with quick sort) while linear search is given the data as it is.")]),
 ("The sort and search used inside the compiler and simulator", [
  ("p", "The same ideas run in the real program, with templates so they work on any record type. quickSort(array, less) and binarySearch(array, key, compare) are in TableSort.cpp and TableSearch.cpp."),
  ("steps", [
    "quickSort uses a Hoare partition: the pivot is the middle element; one index moves right past smaller items, the other moves left past larger items, and the two items they stop on are swapped. This repeats until the indexes cross.",
    "It then recurses into the smaller part and loops on the larger part, so the recursion depth stays O(log n) even for bad data.",
    "binarySearch calls compare(element, key), which returns negative, zero or positive, and fills an optional list with every index it probes, so the probes can be shown."]),
  ("table", ["Where", "What is sorted and searched"], [
    ["core/Instruction.cpp: OpcodeTable", "The table of instructions is sorted by mnemonic once at start-up; every mnemonic the assembler meets is looked up with binary search, and the probes are kept for display."],
    ["engine/lang/Compiler.cpp: lexer", "Each word of the source is looked up in the sorted keyword table with binary search, to tell a keyword from a variable name."],
    ["app/Sim.cpp: memory view", "The memory rows are sorted by address, and each address is looked up in the symbols (also sorted by address) with binary search to show the variable's name."]])]),
 ("Where it is implemented", [
  ("table", ["File", "What is in it"], [
    ["engine/ds/Sorting.h, Sorting.cpp", "BubbleSort, InsertionSort and QuickSort, each recording its steps."],
    ["engine/ds/Searching.h, Searching.cpp", "LinearSearch, BinarySearch and FibonacciSearch, each recording its probes."],
    ["engine/ds/TableSort.cpp, TableSearch.cpp", "The template quickSort and binarySearch."],
    ["app/DataLabView.cpp", "The Sorting and Searching tabs: choose an algorithm, play the steps, compare all three."],
    ["core/Instruction.cpp, engine/lang/Compiler.cpp, app/Sim.cpp", "The three real uses listed above."],
    ["CO4_SearchingSorting/sorting_algorithms.cpp, searching_algorithms.cpp", "The sorting and searching algorithms as plain functions."],
    ["CO4_SearchingSorting/opcode_table_lookup.cpp, memory_by_address.cpp", "A sorted opcode table with binary search, and memory rows looked up by address."]])]),
]),

(5, "CO5_Hashing", "Apply suitable hashing techniques for solving given problems.", [
 ("The idea", [
  ("p", "A hash function turns a key (here a word) into a number, and the number, taken modulo the table size, is the slot where the key is stored. Looking up the same key gives the same slot, so a lookup takes the same time however many keys there are. "
        "Two keys can land in the same slot (a collision), so a method for resolving collisions is needed. The program implements three and lets the user choose.")]),
 ("The hash function", [
  ("code", "h(s) = s0 * 31^(n-1) + s1 * 31^(n-2) + ... + s(n-1)       (computed with Horner's rule: h = h * 31 + character)\nhome slot = h(s) mod m"),
  ("p", "Every character changes the result and its position matters, so \"abc\" and \"cab\" give different numbers. The table size m is always a prime (nextPrime finds it), because a prime size spreads the remainders evenly. "
        "Worked example: h(\"s\") = 115 and 115 mod 17 = 13; h(\"rate\") = 3 493 088 and 3 493 088 mod 17 = 13 as well. All of the variables s, rate, count, left, index and result have home slot 13 in a table of 17 slots, which is why the Data Lab sample uses them to show collisions.")]),
 ("The three collision methods", [
  ("table", ["Method", "Slot order", "How it works"], [
    ["Linear probing", "(h + i) mod m", "On a collision try the next slot, then the next. Simple, but colliding keys pile up in one solid run (primary clustering), so later keys must step over the whole run."],
    ["Quadratic probing", "(h + i^2) mod m", "Try h, h+1, h+4, h+9 ... The steps get bigger, so colliding keys spread apart and the runs do not grow together. It can miss free slots when the table is too full."],
    ["Separate chaining", "h mod m, then a chain", "Every slot holds a linked list. A collision just adds the key to the end of that slot's chain; no other slot is touched."]]),
  ("p", "Worked example in a table of 17 slots, inserting s, rate, count, left (all home slot 13): linear probing puts them in slots 13, 14, 15, 16. Quadratic probing puts s in 13, rate in 14 (13 + 1), count in 0 (13 + 4 = 17, which wraps to 0), and left in 5 (13 + 9 = 22, which wraps to 5). Chaining keeps all four in slot 13, at chain positions 0, 1, 2 and 3.")]),
 ("Insert and find", [
  ("steps", [
    "Compute the raw hash h(key) and the home slot = h mod capacity. This is recorded in a ProbeTrace together with the capacity.",
    "Probing tables: for i = 0, 1, 2 ... compute the slot from the formula and record it. If the slot is empty, store the key there. If it holds the same key, update the value. Otherwise this is a collision (counted once, at i = 0) and the loop goes on.",
    "Chaining: go to the chain of the home slot and walk it. If the key is found, update it; otherwise append a new node at the end and record its chain position.",
    "find follows exactly the same slots. In a probing table an empty slot means the key is not there; in a chain the end of the chain means the same."]),
  ("p", "The ProbeTrace (raw hash, capacity, home slot, every slot examined, final slot, chain position, whether a collision or a rehash happened) is what the Data Lab animates.")]),
 ("Growing the table (rehash)", [
  ("table", ["Method", "When it grows", "New size"], [
    ["Linear, quadratic", "before an insert that would make the load (items / slots) go above 0.7", "nextPrime(2 x old size + 1)"],
    ["Chaining", "before an insert that would make the item count more than twice the slot count (chains too long)", "nextPrime(2 x old size + 1)"]]),
  ("p", "A rehash makes a new prime-sized array and inserts every key again, because the home slot depends on the size (h mod m changes when m does). Quadratic probing can fail to find a free slot even below the limit, so its insert loop also rehashes and tries again if no slot was found. "
        "Work done during a rehash is not counted in the user's probe and collision totals. The table starts with 7 slots.")]),
 ("Where hashing is used in the program", [
  ("table", ["Use", "Structure", "Key to value"], [
    ["Compiler symbol table", "HashTable (the method the user chose)", "variable name to its memory address"],
    ["Assembler label table", "HashTable (the method the user chose)", "label to instruction index"],
    ["Register names", "HashMap<string, int>", "\"rax\" to 0 ... \"r15\" to 15"],
    ["Sparse memory", "HashMap<uint64, int64>", "address to the 64-bit value stored there"],
    ["Micro-code ROM", "HashMap<string, int>", "routine key to micro address"],
    ["Translation cache", "HashMap<string, string>", "statement text to the assembly already made for it"],
    ["Profiler counts", "HashMap<string, int>", "name to how many times it was counted"]]),
  ("p", "HashMap is a generic separate-chaining map. Integer keys use a multiplicative hash and strings use FNV-1a; its buckets double when the load goes above 1, so lookup stays about O(1) on average.")]),
 ("Where it is implemented", [
  ("table", ["File", "What is in it"], [
    ["engine/ds/HashTable.h, HashTable.cpp", "The abstract HashTable, ProbeTrace, polyHash, isPrime, nextPrime and the factory that makes the chosen method."],
    ["engine/ds/OpenAddressing.h, .cpp", "LinearProbingTable and QuadraticProbingTable: probeIndex, insert, find, place and rehash."],
    ["engine/ds/Chaining.h, .cpp", "ChainingTable: insert, find and rehash with chains."],
    ["engine/ds/HashMap.h, HashMap.cpp", "The generic HashMap with put, find, erase and forEach."],
    ["engine/lang/Compiler.cpp, core/Assembler.cpp", "The symbol table and the label table."],
    ["app/DataLabView.cpp", "The hashing tab: insert keys, switch between Linear, Quadratic and Chaining, see the probes."],
    ["CO5_Hashing/collision_strategies.cpp, symbol_table.cpp, lru_cache.cpp, word_counter.cpp", "The strategies, a symbol table, a least-recently-used cache and a word counter written as plain functions."]])]),
]),
]
