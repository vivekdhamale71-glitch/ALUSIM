#!/usr/bin/env python3
"""Builds one explanation PDF per course outcome.

Every PDF is written into its CO folder, e.g.
    src/COA_2310221L/CO3_ComputerArithmetic/CO3_Explanation.pdf

Run from the repository root:   python3 docs/make_co_pdfs.py
Needs Google Chrome or Chromium (used headless to print HTML to PDF).
"""
import html
import os
import shutil
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

SUBJECTS = {
    "PL": ("PL_2310214L", "Programming Lab - Data Structures", "2310214L"),
    "COA": ("COA_2310221L", "Computer Organization & Architecture", "2310221L"),
    "CG": ("CG_2310218L", "Computer Graphics (OpenGL)", "2310218L"),
    "PSOOP": ("PSOOP_2310261L", "Problem Solving using OOP", "2310261L"),
}


def code(s):
    return "<pre>" + html.escape(s.strip("\n")) + "</pre>"


def table(head, rows):
    out = "<table><tr>" + "".join(f"<th>{h}</th>" for h in head) + "</tr>"
    for r in rows:
        out += "<tr>" + "".join(f"<td>{c}</td>" for c in r) + "</tr>"
    return out + "</table>"


def ul(items):
    return "<ul>" + "".join(f"<li>{i}</li>" for i in items) + "</ul>"


# --------------------------------------------------------------------------
# content
# --------------------------------------------------------------------------
COS = []


def co(subject, num, folder, statement, summary, where, files, how, viva, example=""):
    COS.append(dict(subject=subject, num=num, folder=folder, statement=statement, summary=summary,
                    where=where, files=files, how=how, example=example, viva=viva))


# ============================== PL ==========================================
co("PL", 1, "CO1_Arrays", "Develop applications using arrays.",
   "The CPU's register file is an array: 16 registers addressed by number, so reading or writing "
   "RAX, RBX ... R15 is one O(1) index operation.",
   ["Registers panel (right side of every screen): the 16 rows are the 16 array elements.",
    "Every instruction that reads or writes a register goes through this array."],
   [("CO1_Arrays/FixedArray.h", "class <code>FixedArray&lt;T, N&gt;</code>", "the array type: size fixed at compile time"),
    ("CO1_Arrays/FixedArray.cpp", "<code>at()</code>, <code>fill()</code>, constructor", "bounds-checked access and traversal"),
    ("core/CPU.h", "<code>FixedArray&lt;Register64, REG_COUNT&gt; regs_</code>", "the register file of the simulated CPU")],
   """
<p><b>Why an array.</b> Registers have fixed numbers (RAX = 0, RCX = 1 ... R15 = 15) and there are always
exactly 16 of them. An array stores them contiguously, so the address of register <i>i</i> is
<code>base + i &times; sizeof(Register64)</code>: no searching, no pointers.</p>
<p><b>Two kinds of access.</b></p>
""" + table(["Function", "Check", "Used for"], [
       ["<code>operator[](i)</code>", "none, O(1)", "the CPU's hot path (index already validated by the assembler)"],
       ["<code>at(i)</code>", "throws <code>std::out_of_range</code> if i &ge; N", "safe access from outside the CPU"]]) + """
<p><b>Template parameters.</b> <code>T</code> is the element type and <code>N</code> the size. Because N is a
compile-time constant the storage is a plain C array <code>T data_[N]</code> inside the object: no heap
allocation, and <code>size()</code> is <code>constexpr</code>.</p>
<p><b>Traversal.</b> <code>fill(v)</code> visits every element once (O(N)). It resets the register file when a
program is reloaded. <code>begin()/end()</code> let a range-for loop walk the array.</p>
""",
   [("Why an array and not a linked list for registers?", "The count is fixed and access is by number. An array gives O(1) random access; a list would need O(n) walking."),
    ("What is the difference between operator[] and at()?", "operator[] does no check (fast). at() checks the index and throws std::out_of_range."),
    ("Where is the array stored?", "Inside the CPU object itself (T data_[N]), not on the heap, because N is known at compile time."),
    ("Complexity of reading a register?", "O(1): one address calculation."),
    ("Other arrays in the project?", "The opcode table (sorted array), the waveform columns, the hash-table slot arrays and the COA decoder ROM.")],
   code("""
FixedArray<Register64, 16> regs_;      // core/CPU.h
regs_[RAX] = Register64(42);           // RAX is index 0
regs_.at(16);                          // throws: index 16 out of range
"""))

co("PL", 2, "CO2_LinkedLists", "Choose suitable type of linked list for various applications.",
   "Three different linked lists, each picked for its shape: singly for the lexer's token stream, "
   "doubly for the Step Back history, circular for the FETCH-DECODE-EXECUTE-MEMORY-WRITEBACK cycle.",
   ["Compiler screen: the token stream is a singly linked list.",
    "Step Back (left arrow / F9): walks back through the doubly linked history.",
    "Datapath screen: the phase timeline IF ID EX MEM WB is drawn by walking the circular list, and the "
    "Control Signals card shows “EXECUTE → MEMORY”, the next node of the ring."],
   [("SinglyLinkedList.h / .cpp", "<code>pushBack</code>, <code>clear</code>, iterator", "token stream (PSOOP Compiler.cpp)"),
    ("DoublyLinkedList.h / .cpp", "<code>pushBack</code>, <code>truncateAfter</code>", "execution history (app/Sim.cpp)"),
    ("CircularLinkedList.h / .cpp", "<code>insert</code>, <code>find</code>, <code>clear</code>", "instruction-cycle ring (app/Sim.cpp, DatapathView.cpp)")],
   """
<p><b>Singly linked: token stream.</b> The lexer only ever appends tokens and the parser reads them once
from front to back. With a tail pointer <code>pushBack</code> is O(1), and a node needs only one link.</p>
<p><b>Doubly linked: undo history.</b> Each node holds the CPU snapshot taken before an instruction ran.
Step Back needs the node <i>before</i> the tail, so every node has a <code>prev</code> link:</p>
""" + code("""
auto* tail = history.tail();
cpu.restore(tail->value.before);        // undo the last instruction
history.truncateAfter(tail->prev);      // delete that node, prev becomes the tail
""") + """
<p><b>Circular: instruction cycle.</b> After WRITEBACK the CPU goes back to FETCH, which is exactly a ring:
the last node points to the first, so "next phase" never needs an end-of-list case. Only a tail pointer
is stored (head = tail->next), which makes insertion at the end O(1).</p>
""" + code("""
auto* node = sim.phaseRing.find(currentPhase);   // one lap at most
Phase next = node->next->value;                    // WB -> FETCH wraps automatically
""") + table(["List", "Links per node", "Chosen because"], [
       ["Singly", "next", "append + one forward pass"],
       ["Doubly", "prev, next", "must move backwards (undo)"],
       ["Circular", "next (last -> first)", "the data is a cycle"]]),
   [("Why not use a doubly linked list everywhere?", "It costs an extra pointer per node and extra work per insert. The token stream never goes backwards, so a singly list is enough."),
    ("How does Step Back find the previous state in O(1)?", "Through tail->prev; a singly list would need an O(n) walk from the head."),
    ("How do you detect the end of a circular list?", "Stop after size() nodes, or when you reach the starting node again (find() does one lap)."),
    ("Why store only the tail in the circular list?", "head = tail->next, so both ends are reachable and inserting at the end is O(1)."),
    ("What does truncateAfter do?", "Deletes every node after the given one; with nullptr it clears the whole list.")])

co("PL", 3, "CO3_StackQueue", "Build scenario based applications using stack and queue.",
   "The compiler turns infix expressions into postfix with the shunting-yard algorithm, which needs an operator "
   "stack and an output queue; code generation then uses a value stack and a stack of free registers. "
   "The waveform is a circular queue.",
   ["Compiler screen: the shunting-yard animation shows the operator stack and the output queue.",
    "Datapath screen: the control-signal waveform keeps the last 56 cycles in a circular queue."],
   [("Stack.h / Stack.cpp", "<code>push</code>, <code>pop</code>, <code>top</code>, <code>grow</code>", "dynamic-array stack (LIFO)"),
    ("Queue.h / Queue.cpp", "<code>enqueue</code>, <code>dequeue</code>, <code>front</code>, <code>grow</code>", "circular queue (FIFO), optional overwrite"),
    ("PSOOP .../Compiler.cpp", "<code>shunt()</code>, <code>genPostfix()</code>", "the scenario: expression compilation"),
    ("app/Sim.h", "<code>CircularQueue&lt;SignalSample&gt; wave{56, true}</code>", "rolling waveform buffer")],
   """
<p><b>Stack.</b> Stored in an array; <code>size_</code> is the top. Push writes <code>data_[size_++]</code>, pop
reads <code>data_[--size_]</code>, both O(1). When full, <code>grow()</code> doubles the array (amortised O(1)).
Popping an empty stack throws <code>std::underflow_error</code>.</p>
<p><b>Circular queue.</b> Front is <code>head_</code>, rear is <code>(head_ + count_) % capacity</code>. Both
indices move forward and wrap round, so no element is ever shifted. In <i>overwrite</i> mode a full queue
drops its oldest element: exactly what a logic-analyser waveform needs.</p>
<p><b>Shunting-yard</b> (infix to postfix):</p>
""" + ul(["operand: enqueue it to the output queue",
          "operator: while the stack top has higher or equal precedence, pop it to the output; then push",
          "( : push;  ) : pop to the output until (",
          "end: pop everything left"]) + """
<p><b>Code generation</b> reads the postfix queue: operands are pushed on a value stack; an operator pops two
values, emits an instruction into a free register (taken from the register stack) and pushes the result.</p>
""",
   [("Why is a stack natural for operators?", "The most recent, highest-precedence operator must be applied first: last in, first out."),
    ("Why a circular queue instead of a simple array queue?", "Dequeue would otherwise leave unused space at the front or force shifting; wrapping reuses it in O(1)."),
    ("Full vs empty in the circular queue?", "count_ is kept separately: empty when count_ == 0, full when count_ == capacity."),
    ("What happens on overflow?", "Stack: the array doubles. Queue: grows, or in overwrite mode the oldest value is dropped."),
    ("Postfix of (a + b) * c?", "a b + c *")],
   table(["Token", "Action", "Operator stack", "Output queue"], [
       ["a", "output", "", "a"], ["+", "push", "+", "a"], ["b", "output", "+", "a b"],
       ["*", "higher than + : push", "+ *", "a b"], ["2", "output", "+ *", "a b 2"],
       ["end", "pop all", "", "a b 2 * +"]]) + "<p>Expression <code>a + b * 2</code> &rarr; postfix <code>a b 2 * +</code>.</p>")

co("PL", 4, "CO4_SearchingSorting", "Solve given problems using searching and sorting techniques.",
   "The assembler sorts its opcode table once with quick sort and then finds every mnemonic with binary search; "
   "the lexer finds keywords the same way. The Data Lab animates bubble, insertion and quick sort on the profiler's "
   "instruction counts, and linear, binary and Fibonacci search on data memory.",
   ["Data Lab screen, Sorting tab: compares and swaps of the three sorts, with counts.",
    "Data Lab screen, Searching tab: each probe of the three searches on memory.",
    "Every assembled instruction: its mnemonic is found by binary search."],
   [("Sorting.h / Sorting.cpp", "<code>BubbleSort</code>, <code>InsertionSort</code>, <code>QuickSort</code>", "Data Lab sorts (record every step)"),
    ("Searching.h / Searching.cpp", "<code>LinearSearch</code>, <code>BinarySearch</code>, <code>FibonacciSearch</code>", "Data Lab searches"),
    ("TableSort.h / .cpp", "<code>quickSort&lt;T&gt;</code>", "sorts the opcode table at start-up"),
    ("TableSearch.h / .cpp", "<code>binarySearch&lt;T&gt;</code>", "mnemonic and keyword lookup")],
   table(["Algorithm", "Idea", "Time"], [
       ["Bubble sort", "swap neighbours; the largest bubbles to the end; stop early if no swap", "O(n<sup>2</sup>), O(n) if sorted"],
       ["Insertion sort", "insert each element into the sorted prefix", "O(n<sup>2</sup>), fast on nearly sorted data"],
       ["Quick sort", "partition around a pivot, sort both sides", "O(n log n) average, O(n<sup>2</sup>) worst"],
       ["Linear search", "compare every element", "O(n), any order"],
       ["Binary search", "compare with the middle, discard half", "O(log n), sorted data"],
       ["Fibonacci search", "split by Fibonacci numbers, only + and -", "O(log n), sorted data"]]) + """
<p><b>Sort once, search many times.</b> The opcode table has 33 entries and is searched for every line of
every program. Sorting it once (O(n log n)) makes each lookup O(log n), at most 6 comparisons instead of up to 33.</p>
""" + code("""
quickSort(table_, [](const OpcodeInfo& a, const OpcodeInfo& b) { return a.mnemonic < b.mnemonic; });
int i = binarySearch(table_, mnem, cmp);     // core/Instruction.cpp
""") + """
<p><b>Why classes for the Data Lab.</b> <code>Sorter</code> and <code>Searcher</code> are abstract base classes, so
the screen can run any algorithm through the same <code>sort()</code> / <code>search()</code> call and draw the
recorded steps (Strategy pattern).</p>
""",
   [("Why must the data be sorted for binary search?", "Discarding half the range is only valid if everything left of the middle is smaller."),
    ("Why mid = lo + (hi - lo) / 2?", "It avoids overflow of lo + hi for very large indices."),
    ("When is insertion sort better than quick sort?", "On small or nearly sorted arrays: few shifts and no recursion."),
    ("How does the generic quickSort keep recursion shallow?", "It recurses into the smaller part and loops on the larger, so the depth is O(log n)."),
    ("Advantage of Fibonacci search?", "It uses only addition and subtraction to find split points; no division.")])

co("PL", 5, "CO5_Hashing", "Apply suitable hashing techniques for solving given problems.",
   "Variable names (compiler symbol table) and labels (assembler) are stored in a hash table whose collision "
   "strategy the user chooses: linear probing, quadratic probing or separate chaining. A generic chained "
   "HashMap provides O(1) lookups everywhere else.",
   ["Data Lab screen, Hashing tab: inserts the program's variables and shows home slot, probes, collisions and load factor.",
    "Compiler screen: the symbol table panel.",
    "Compiler screen: the hash-method selector changes the table used by the compiler and the assembler."],
   [("HashTable.h / .cpp", "<code>polyHash</code>, <code>nextPrime</code>, <code>makeHashTable</code>", "base class, hash function, factory"),
    ("OpenAddressing.h / .cpp", "<code>insert</code>, <code>find</code>, <code>place</code>, <code>rehash</code>, <code>probeIndex</code>", "linear and quadratic probing"),
    ("Chaining.h / .cpp", "<code>insert</code>, <code>find</code>, <code>rehash</code>", "separate chaining"),
    ("HashMap.h / .cpp", "<code>put</code>, <code>find</code>, <code>erase</code>, <code>rehash</code>", "generic map: opcodes, registers, memory, micro-code ROM, cache")],
   """
<p><b>Hash function.</b> <code>h(s) = s<sub>0</sub>&middot;31<sup>n-1</sup> + ... + s<sub>n-1</sub></code> (Horner's rule),
home slot = <code>h mod m</code>. Table sizes <code>m</code> are prime, so keys spread evenly.</p>
""" + table(["Method", "Slot for attempt i", "Grows when"], [
       ["Linear probing", "(h + i) mod m", "load factor &gt; 0.7"],
       ["Quadratic probing", "(h + i<sup>2</sup>) mod m", "load factor &gt; 0.7, or no free slot found"],
       ["Separate chaining", "h mod m, then walk the chain", "average chain length &gt; 2"]]) + """
<p><b>Rehashing.</b> The array is replaced by one of size <code>nextPrime(2m + 1)</code> and every key is inserted
again, because its home slot <code>h mod m</code> changes with m.</p>
<p><b>HashMap&lt;K,V&gt;.</b> A template with separate chaining. Strings are hashed with FNV-1a and integers with
Knuth's multiplicative hash. Uses: mnemonic &rarr; factory, register name &rarr; number, address &rarr; memory word
(sparse memory), micro-routine &rarr; address, statement &rarr; cached assembly.</p>
""",
   [("Primary clustering?", "With linear probing, occupied slots form runs that keep growing; quadratic probing jumps over them."),
    ("Why keep the load factor below 0.7?", "Probe sequences in open addressing grow quickly as the table fills."),
    ("Why prime table sizes?", "They spread keys better and let quadratic probing reach more slots."),
    ("Chaining vs open addressing?", "Chaining never fills up and deletes easily, but needs extra nodes; open addressing is compact and cache friendly."),
    ("Why does memory use a HashMap?", "The address space is huge but only a few addresses are used: a sparse map stores just those.")],
   "<p>Inserting <code>x</code>, <code>sum</code>, <code>i</code> into a linear-probing table of size 7: each key's home slot "
   "is <code>polyHash(key) mod 7</code>; if that slot is taken the next one is tried, and the Data Lab shows every probe.</p>")

# ============================== COA =========================================
co("COA", 1, "CO1_Architecture", "Explicate the architecture and instruction set of the 80386 microprocessor.",
   "The simulator's ALU is written in x86-64 assembly and runs on the real processor's ALU: the simulated flags "
   "are loaded into RFLAGS, the real ADD / SUB / IMUL / AND / SHL ... runs, and the processor's own CF ZF SF OF are read back.",
   ["Every arithmetic or logic instruction of every program goes through <code>vas_alu</code>.",
    "Registers panel: RAX with its EAX / AX / AH / AL views (8086 &rarr; 80386 &rarr; x86-64).",
    "The CF ZF SF OF lamps under the registers show the flags returned by the real CPU."],
   [("CO1_Architecture/alu.asm", "<code>vas_alu(op, a, b, &amp;rflags)</code>", "the ALU: 14 operations + flags"),
    ("abi.inc", "<code>ENTRY</code> / <code>RETURN</code>", "System V (Linux) and Windows x64 calling conventions"),
    ("coa_asm.h", "<code>extern \"C\"</code> declarations", "how C++ calls the assembly"),
    ("core/Register64.h", "<code>ALU::compute</code>, sub-register views", "C++ side: calls vas_alu")],
   """
<p><b>Calling convention.</b> C++ passes <code>op</code> in RDI, <code>a</code> in RSI, <code>b</code> in RDX and a
pointer to the flags in RCX; the result comes back in RAX.</p>
<p><b>How one operation runs</b></p>
""" + code("""
lea    r10, [rel alu_table]
movsxd r11, dword [r10 + rdi*4]   ; pick the handler for this op (jump table)
add    r10, r11
mov    rax, rsi                    ; rax = a
push   r9                          ; r9 = simulated flags (CF ZF SF OF only)
popfq                              ; -> into the real RFLAGS
jmp    r10                         ; JMP does not change flags
op_add:
    add  rax, rdx                  ; the real instruction sets the flags
    pushfq                         ; read RFLAGS back
    pop  r10
    and  r10, 0x8C1                ; keep OF(11) SF(7) ZF(6) CF(0)
""") + """
<p>Because the real instruction runs, the subtle x86 rules come for free: <code>INC</code>/<code>DEC</code> keep CF,
<code>NOT</code> changes no flags, logic operations clear CF and OF, and a shift by 0 leaves all flags alone.
Two cases need extra care: <code>IMUL</code> leaves ZF/SF undefined, so they are taken from a <code>TEST</code>
of the product, and the CPU's divide is guarded so <code>INT64_MIN / -1</code> cannot trap.</p>
""" + table(["Flag", "Bit", "Meaning"], [
       ["CF", "0", "unsigned carry / borrow out of bit 63"], ["ZF", "6", "result is zero"],
       ["SF", "7", "bit 63 of the result (negative)"], ["OF", "11", "signed overflow"]]),
   [("Why load the flags with POPFQ first?", "Some instructions leave some flags unchanged (INC keeps CF). The old values must already be in RFLAGS for that to work."),
    ("Why is the jump target computed before POPFQ?", "CMP/ADD would destroy the freshly loaded flags; LEA, MOV and JMP do not touch flags."),
    ("CF vs OF?", "CF is overflow for unsigned numbers, OF for signed. 0x7FFF...F + 1 sets OF but not CF."),
    ("What does writing EAX do to RAX?", "Clears bits 63..32. Writing AX or AL leaves the other bits unchanged."),
    ("Why the 0x8C1 mask?", "Only the four status flags are simulated; it also stops the code from setting TF or DF.")],
   table(["a", "b", "op", "result", "CF ZF SF OF"], [
       ["5", "5", "SUB", "0", "0 1 0 0"], ["0", "1", "SUB", "-1", "1 0 1 0"],
       ["0x7FFFFFFFFFFFFFFF", "1", "ADD", "0x8000000000000000", "0 0 1 1"],
       ["0xFFFFFFFFFFFFFFFF", "1", "ADD", "0", "1 1 0 0"]]))

co("COA", 2, "CO2_AssemblyISA", "Develop assembly language programs using 80386 instruction set.",
   "Programs written in assembly (or generated by the compiler) are assembled by a two-pass assembler and run on "
   "the simulated CPU. The instruction semantics that depend on processor details are themselves assembly: "
   "branch conditions use the real SETcc, IDIV the real IDIV, CQO the real CQO.",
   ["Compiler screen in Assembly mode: write x86-64 directly; errors point at the line.",
    "Every Jcc, IDIV and CQO executed by a program calls into isa.asm."],
   [("CO2_AssemblyISA/isa.asm", "<code>vas_condition(cc, rflags)</code>", "is the branch taken? (SETE SETNE SETG SETGE SETL SETLE)"),
    ("", "<code>vas_idiv(rdx, rax, divisor, out)</code>", "signed 128 / 64 division with #DE detection"),
    ("", "<code>vas_cqo(rax)</code>", "sign-extend RAX into RDX"),
    ("core/Assembler.cpp", "two-pass assembler", "pass 1 labels, pass 2 instructions"),
    ("core/Instruction.cpp", "33-instruction ISA", "calls isa.asm from conditionMet() and IDIV / CQO")],
   """
<p><b>Branch conditions.</b> The simulated flags are loaded into RFLAGS and the matching SETcc sets AL to 0 or 1,
so the result follows the processor's own definition of each condition:</p>
""" + table(["Mnemonic", "cc", "Taken when", "Instruction"], [
       ["JE / JZ", "1", "ZF = 1", "SETE"], ["JNE / JNZ", "2", "ZF = 0", "SETNE"],
       ["JG", "3", "ZF = 0 and SF = OF", "SETG"], ["JGE", "4", "SF = OF", "SETGE"],
       ["JL", "5", "SF &ne; OF", "SETL"], ["JLE", "6", "ZF = 1 or SF &ne; OF", "SETLE"]]) + """
<p><b>IDIV.</b> RDX:RAX (128 bits) divided by a 64-bit divisor. On real hardware a zero divisor, or a quotient that
does not fit in 64 bits, raises a divide error (#DE). <code>vas_idiv</code> first takes the magnitudes (128-bit negate
with NOT / ADD / ADC), checks <code>|high| &lt; |divisor|</code> and the sign range of the quotient, and only then runs
the real <code>IDIV</code>, which can no longer trap. It returns 0 (ok), 1 (divide by zero) or 2 (overflow).</p>
<p><b>Why CQO before IDIV.</b> IDIV divides the 128-bit RDX:RAX. CQO copies the sign bit of RAX into every bit of
RDX so a negative 64-bit dividend becomes the correct 128-bit value.</p>
""",
   [("Two passes in the assembler, why?", "A jump may refer to a label further down; pass 1 records every label's address, pass 2 encodes the instructions."),
    ("JG vs JA?", "JG is for signed comparisons (SF = OF), JA for unsigned (CF = 0 and ZF = 0)."),
    ("What happens if CQO is forgotten?", "RDX may hold garbage, the 128-bit dividend is wrong and the quotient can overflow: #DE."),
    ("Why not run IDIV directly?", "An overflow would crash the simulator with SIGFPE; the checks turn it into a clean error message."),
    ("What does SETcc do?", "Writes 1 to an 8-bit register if the condition holds, otherwise 0.")],
   code("""
    mov  rax, -17
    cqo                  ; RDX = 0xFFFF...FFFF
    mov  rbx, 5
    idiv rbx             ; RAX = -3 (quotient), RDX = -2 (remainder)
    cmp  rax, 0
    jl   negative        ; taken: SF != OF
"""))

co("COA", 3, "CO3_ComputerArithmetic",
   "Illustrate processor level details about arithmetic operations using computer arithmetic algorithms.",
   "Booth's multiplication, add-and-shift multiplication, restoring division and binary / BCD conversion are written in "
   "assembly on n-bit registers. Each records every step (A, Q, Q-1 or C, operation) into an array that the "
   "Arithmetic screen animates bit by bit.",
   ["Arithmetic screen: choose Booth's / Add &amp; Shift / Restoring Division / Hex &harr; BCD; the 3D register cubes and the step table come from these functions.",
    "“From last IMUL / IDIV” takes the operands from the program that just ran."],
   [("CO3_ComputerArithmetic/arithmetic.asm", "<code>vas_booth</code>", "signed multiplication"),
    ("", "<code>vas_add_shift</code>", "unsigned multiplication"),
    ("", "<code>vas_restoring_div</code>", "unsigned division"),
    ("", "<code>vas_bin_to_bcd</code>, <code>vas_bcd_to_bin</code>", "number systems"),
    ("core/Arithmetic.cpp", "<code>boothMultiply</code> ...", "turns the step records into text for the screen")],
   """
<p><b>Booth's algorithm</b> (signed M &times; Q). Look at the last bit of Q and the extra bit Q-1:</p>
""" + table(["Q0 Q-1", "Action"], [["0 0 / 1 1", "nothing"], ["1 0", "A = A - M"], ["0 1", "A = A + M"]]) + """
<p>then arithmetic shift right A:Q:Q-1 (the sign bit of A is kept). After n cycles A:Q holds the 2n-bit product.</p>
<p><b>Add-and-shift</b> (unsigned): if Q0 = 1 then C,A = A + M; then shift C:A:Q right. <b>Restoring division:</b>
shift A:Q left, A = A - M; if A is negative restore it (A = A + M) and set Q0 = 0, else set Q0 = 1.</p>
<p><b>Each step is recorded</b> as a 32-byte record <code>{A, Q, extra, kind}</code> by a macro:</p>
""" + code("""
%macro RECORD 1
    mov  rbx, rax          ; rax = number of records so far
    shl  rbx, 5            ; x 32 bytes
    add  rbx, rcx          ; rcx = output array
    mov  [rbx], r10        ; A
    mov  [rbx + 8], r11    ; Q
    mov  [rbx + 16], r12   ; Q-1 or carry
    mov  qword [rbx + 24], %1
    inc  rax
%endmacro
"""),
   [("Why does Booth need an extra bit Q-1?", "The pair (Q0, Q-1) detects the start (10) and end (01) of a run of 1s; a run needs only one subtract and one add."),
    ("Why arithmetic and not logical shift in Booth?", "A holds a signed partial product; the sign bit must be copied in to keep its value."),
    ("Why is A one bit wider in restoring division?", "After A - M the extra top bit is the sign that tells whether to restore."),
    ("What is the difference between restoring and non-restoring division?", "Non-restoring skips the restore step and adds M in the next cycle instead."),
    ("What is packed BCD?", "One decimal digit per 4-bit nibble: 1234 is 0001 0010 0011 0100 = 0x1234.")],
   "<p><b>Booth 7 &times; -3 with 4-bit registers</b> (M = 0111, -M = 1001):</p>" + table(
       ["Step", "Operation", "A", "Q", "Q-1"], [
           ["0", "initial", "0000", "1101", "0"], ["1", "A = A - M", "1001", "1101", "0"], ["1", "ASR", "1100", "1110", "1"],
           ["2", "A = A + M", "0011", "1110", "1"], ["2", "ASR", "0001", "1111", "0"], ["3", "A = A - M", "1010", "1111", "0"],
           ["3", "ASR", "1101", "0111", "1"], ["4", "ASR (11: nothing)", "1110", "1011", "1"]]) +
   "<p>A:Q = 1110 1011 = -21. &nbsp; <b>Restoring 11 / 3</b> (4-bit) ends with Q = 0011 = 3, A = 00010 = 2.</p>")

co("COA", 4, "CO4_ControlUnitPipeline",
   "Demonstrate the different ways to generate control signals and their organization techniques and instruction pipelining of computer system.",
   "The hardwired control unit's decoder ROM is in assembly: one 32-bit word per instruction form holding the 11 "
   "control signals and the ALU operation. The 5-stage pipeline's timing rules (operand readiness with and without "
   "forwarding, and stage scheduling with stalls) are also assembly functions.",
   ["Datapath screen: Hardwired / Microprogrammed switch, the decoder table, the 11-signal vector each cycle and the waveform.",
    "Pipeline screen: space-time diagram, stalls, forwarding arrows, CPI and speed-up."],
   [("CO4_ControlUnitPipeline/control_unit.asm", "<code>vas_decode(form)</code>, <code>vas_decode_rows()</code>", "hardwired decoder ROM"),
    ("", "<code>vas_operand_ready(mem, wb, fwd, load)</code>", "earliest EX cycle for a dependent instruction"),
    ("", "<code>vas_schedule(prev, fetch, ready, out)</code>", "IF ID EX MEM WB start cycles + stall count"),
    ("core/ControlUnit.cpp", "<code>MicroprogrammedControlUnit</code>", "control store, micro-PC, dispatch ROM"),
    ("core/Pipeline.cpp", "<code>PipelineSimulator::run</code>", "builds the diagram from the execution trace")],
   """
<p><b>Hardwired: one ROM word per instruction form.</b></p>
""" + code("""
;     signals (bits 0..10)                     ALU op (bits 16..23)
dd    RGW | FLW                              | ALU(A_ADD)    ; ALU reg, reg
dd    MRD | RGW | SRC | M2R                  | ALU(A_ADD)    ; MOV reg, [mem]
dd    PCW | MWR | RGW | SRC | JMP | STK      | ALU(A_SUB)    ; CALL
""") + """
<p>Bits: PCWrite IRWrite MemRead MemWrite RegWrite ALUSrc MemToReg Branch Jump FlagWrite StackOp.
Decoding is a single indexed load: <code>mov eax, [decoder_rom + form*4]</code>. The microprogrammed unit instead
steps a micro-PC through a 41-word control store, with a dispatch ROM jumping to each instruction's micro-routine,
which is slower but easy to change.</p>
<p><b>Pipeline timing.</b></p>
""" + table(["Producer", "No forwarding: EX of consumer &ge;", "Forwarding: EX of consumer &ge;"], [
       ["ALU result", "WB + 1", "MEM (EX/MEM bypass)"], ["Load", "WB + 1", "WB (MEM/WB bypass, 1 bubble)"]]) + """
<p><code>vas_schedule</code>: ID = max(IF + 1, prev EX), EX = max(ID + 1, prev MEM, ready), MEM = max(EX + 1, prev WB),
WB = MEM + 1. Stalls = MEM - IF - 3. CPI = cycles / instructions; speed-up = 5n / pipelined cycles.</p>
""",
   [("Hardwired vs microprogrammed?", "Hardwired: signals from logic/ROM in one step, fast, hard to change. Microprogrammed: signals stored as micro-instructions, flexible, slower."),
    ("What is a RAW hazard?", "An instruction reads a register before the previous instruction has written it."),
    ("Why does a load still stall with forwarding?", "Its data exists only after MEM, one cycle too late for the next instruction's EX."),
    ("What is the role of the dispatch ROM?", "It maps the decoded opcode to the first micro-address of that instruction's routine."),
    ("Ideal CPI of a 5-stage pipeline?", "1, after the pipeline fills; stalls and flushes raise it.")])

# ============================== CG ==========================================
co("CG", 1, "CO1_Primitives", "Apply OpenGL graphics primitives to develop graphics applications.",
   "The whole user interface (cards, buttons, wires, waveform, LEDs, text) is drawn with OpenGL primitives; "
   "no UI library is used.",
   ["Every screen: rounded cards are GL_TRIANGLE_FAN, gradients GL_QUADS with per-vertex colour, wires GL_LINES / GL_LINE_STRIP, "
    "signal dots GL_POINTS, text textured GL_QUADS."],
   [("CO1_Primitives/Draw.h / Draw.cpp", "<code>beginFrame</code>, <code>rect</code>, <code>rectGrad</code>, <code>roundRect</code>, <code>line</code>, <code>polyline</code>, <code>circle</code>, <code>ring</code>, <code>triangle</code>, <code>points</code>, <code>bezier</code>, <code>pushClip</code>", "the drawing layer"),
    ("CO1_Primitives/GL.h", "includes", "OpenGL / GLU / GLUT headers for every platform")],
   """
<p><b>2D set-up.</b> <code>beginFrame</code> sets <code>glViewport</code> and <code>glOrtho(0, w, h, 0, -1, 1)</code>, so one unit
is one pixel with the origin at the top-left, and enables alpha blending.</p>
""" + table(["Shape", "Primitive", "Function"], [
       ["Rectangle, gradient", "GL_QUADS (colour per vertex)", "rect, rectGrad"],
       ["Rounded card", "GL_TRIANGLE_FAN round the outline", "roundRect"],
       ["Outline", "GL_LINE_LOOP", "roundRectOutline"],
       ["Wire, path", "GL_LINES, GL_LINE_STRIP", "line, polyline, dashedLine"],
       ["Circle, ring", "GL_TRIANGLE_FAN, GL_TRIANGLE_STRIP", "circle, ring"],
       ["Signal dots", "GL_POINTS", "points"],
       ["Curve", "GL_LINE_STRIP of Bezier points", "bezier"]]) + """
<p><b>Clipping panels.</b> <code>pushClip</code> / <code>popClip</code> keep a stack of rectangles and set
<code>glScissor</code>, so nothing drawn inside a card leaks outside it.</p>
""",
   [("Why glOrtho for the UI?", "It is a parallel projection with no perspective: coordinates map 1:1 to pixels."),
    ("TRIANGLE_FAN vs TRIANGLE_STRIP?", "A fan shares the first vertex (good for convex shapes like rounded rectangles); a strip shares the previous two (good for rings and ribbons)."),
    ("How is a gradient made?", "A different glColor before each vertex; smooth shading interpolates between them."),
    ("What does glScissor do?", "Discards fragments outside a screen rectangle."),
    ("Why blending?", "Shadows, glows and fades use alpha: GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA.")])

co("CG", 2, "CO2_Transformations", "Solve the real time problems using geometric transformations.",
   "The datapath diagram can be zoomed and panned in real time. Its view is a 3 &times; 3 homogeneous matrix "
   "M = T(centre + pan) &middot; S(zoom) &middot; T(-world centre); its inverse maps the mouse back so the zoom stays under the cursor. "
   "The 3D views use the OpenGL matrix stack (glTranslatef, glRotatef, glScalef).",
   ["Datapath screen: mouse wheel zooms about the cursor, drag pans.",
    "3D views: the chip rotates, register cubes flip (glRotatef), packets and bars translate."],
   [("CO2_Transformations/Math.h / Math.cpp", "<code>Mat3::translate</code>, <code>scale</code>, <code>rotate</code>, <code>operator*</code>, <code>apply</code>, <code>inverseAffine</code>", "2D homogeneous transforms"),
    ("app/DatapathView.cpp", "view matrix", "builds and applies M"),
    ("CO4 Scene3D.cpp, app/ChipView.cpp", "glTranslatef / glRotatef / glScalef", "3D transforms on the matrix stack")],
   """
<p><b>Homogeneous coordinates</b> let translation be a matrix multiplication too:</p>
""" + code("""
| x' |   | sx  0  tx |   | x |
| y' | = |  0 sy  ty | * | y |        rotate: | cos -sin 0 |
| 1  |   |  0  0   1 |   | 1 |                | sin  cos 0 |
""") + """
<p><b>Composition</b> (read right to left): move the world centre to the origin, scale, then move to the view centre plus
the pan offset. <code>operator*</code> multiplies rows by columns, so one matrix does all three.</p>
<p><b>Zoom about the cursor.</b> Before zooming, the world point under the mouse is found with
<code>inverseAffine().apply(mouse)</code>; after changing the zoom the pan is adjusted so that world point is again
under the mouse. Inverse of an affine matrix: <code>[A t; 0 1]<sup>-1</sup> = [A<sup>-1</sup>  -A<sup>-1</sup>t; 0 1]</code>.</p>
""",
   [("Why 3 &times; 3 for 2D?", "The extra coordinate makes translation linear, so all transforms compose by multiplication."),
    ("Does the order of transforms matter?", "Yes: matrix multiplication is not commutative. Scale-then-translate differs from translate-then-scale."),
    ("How do you rotate about a point other than the origin?", "T(p) &middot; R &middot; T(-p)."),
    ("Why the inverse matrix?", "To convert a screen position (mouse) back into world coordinates."),
    ("What do glPushMatrix / glPopMatrix do?", "Save and restore the current matrix so one object's transform does not affect the next.")])

co("CG", 3, "CO3_ObjectAlgorithms", "Develop 2D and 3D objects using various algorithms.",
   "Bresenham's line algorithm turns active wires into flowing signal dots, the midpoint circle algorithm draws the flag "
   "LEDs and wire junctions, the scan-line algorithm fills the ALU and multiplexer symbols, and extrusion turns the "
   "same 2D outlines into the 3D chip.",
   ["Datapath screen: dots running along active wires (Bresenham), junction rings (midpoint), filled ALU / MUX shapes (scan-line).",
    "Flag LEDs under the registers (midpoint circle).", "Parse &amp; Chip &rarr; 3D Chip: extruded units."],
   [("CO3_ObjectAlgorithms/Raster.h / .cpp", "<code>bresenhamLine</code>, <code>midpointCircle</code>, <code>scanlineFill</code>", "2D scan conversion"),
    ("CO3_ObjectAlgorithms/Extrude.h / .cpp", "<code>prism</code>, <code>box</code>", "2D outline &rarr; 3D solid")],
   """
<p><b>Bresenham.</b> Integer only. With <code>err = dx + dy</code> (dy negative), each step doubles the error and moves in x,
in y, or both, so the line stays within half a pixel of the true line in every octant.</p>
<p><b>Midpoint circle.</b> Start at (0, r) with decision <code>p = 1 - r</code>. If p &lt; 0 the next pixel is east
(<code>p += 2x + 1</code>), otherwise south-east (<code>p += 2(x - y) + 1</code>). One octant is computed and mirrored 8 ways.</p>
<p><b>Scan-line fill.</b> For every row: intersect each polygon edge with the scan line, sort the x values, fill between
pairs (odd-even rule). Horizontal edges are skipped and an edge's top vertex is excluded so shared vertices are not
counted twice. Works for the concave ALU outline.</p>
<p><b>Extrusion.</b> <code>prism</code> sweeps an outline upwards: one GL_QUAD per edge with a normal perpendicular to it,
and a GL_TRIANGLE_FAN for the top. The 3D chip uses exactly the outlines of the 2D diagram.</p>
""",
   [("Why Bresenham instead of DDA?", "DDA uses floating point and rounding each step; Bresenham uses only integer additions."),
    ("Why compute only one octant of a circle?", "A circle is symmetric: (x, y) gives 8 points (&plusmn;x, &plusmn;y), (&plusmn;y, &plusmn;x)."),
    ("Odd-even rule?", "A point is inside if a ray from it crosses the boundary an odd number of times; pairs of intersections bound the inside spans."),
    ("How are the side normals of the prism found?", "Edge vector (ex, ez) rotated by 90 degrees: (ez, -ex), normalised."),
    ("What is a sweep representation?", "A 3D object defined by moving a 2D shape along a path (here: straight up).")])

co("CG", 4, "CO4_ProjectionViewing", "Apply various methods for projection and viewing it into real world.",
   "Every 3D view sets up a perspective camera with gluPerspective and gluLookAt. The 3D chip has an orbit camera and "
   "an animated 3D &harr; 2D switch: a dolly zoom narrows the field of view while the eye backs away until the "
   "projection becomes parallel (glOrtho top view). gluProject maps 3D points back to the screen for crisp labels.",
   ["Parse &amp; Chip &rarr; 3D Chip: drag to orbit, wheel to zoom, 3D / 2D buttons.",
    "Pipeline (3D conveyor), Arithmetic (register cubes), Data Lab (3D bars) all use Scene3D."],
   [("CO4_ProjectionViewing/Camera.h / .cpp", "<code>orbitCamera</code>", "perspective &harr; orthographic dolly zoom + gluLookAt"),
    ("CO4_ProjectionViewing/Scene3D.h / .cpp", "<code>begin</code>, <code>end</code>, <code>box</code>, <code>project</code>", "3D panel inside the 2D UI")],
   """
<p><b>Perspective vs parallel.</b> <code>gluPerspective(fov, aspect, near, far)</code> makes far objects smaller;
<code>glOrtho</code> keeps sizes regardless of distance (engineering top view).</p>
<p><b>Dolly zoom.</b> The view height at the target is <code>d &middot; tan(fov / 2)</code>. While fov goes from 38&deg; to 2.5&deg;,
the distance becomes <code>d = dist &middot; tan(19&deg;) / tan(fov / 2)</code>, so the chip keeps its size while the
projection lines become almost parallel; at the end the camera switches to <code>glOrtho</code>.</p>
<p><b>Viewing.</b> The eye is on a sphere around the target:</p>
""" + code("""
eye = target + d * (cos(pitch) sin(yaw),  sin(pitch),  cos(pitch) cos(yaw))
gluLookAt(eye, target, up = (0, 1, 0))
""") + """
<p><b>Scene3D.</b> <code>begin(rect, camera)</code> sets a viewport and scissor for one panel, the perspective camera
and two lights; <code>project(x, y, z)</code> calls <code>gluProject</code> with the current matrices so 2D text
can be placed exactly on 3D objects.</p>
""",
   [("What does gluLookAt build?", "The view matrix that moves the world so the eye is at the origin looking down -z."),
    ("Effect of a smaller field of view?", "A telephoto view: less perspective distortion; at the limit, parallel projection."),
    ("Why near and far planes?", "They bound the view volume and set the depth-buffer range."),
    ("What does gluProject do?", "Applies modelview, projection and viewport to a 3D point and returns window coordinates."),
    ("Perspective vs orthographic in one line?", "Perspective: projectors meet at the eye. Orthographic: projectors are parallel.")])

co("CG", 5, "CO5_Clipping", "Implement the suitable clipping algorithms.",
   "When the datapath is zoomed or panned, every wire is clipped to the diagram's viewport with Cohen-Sutherland; "
   "the scrolling waveform traces are clipped to the plot window with Liang-Barsky.",
   ["Datapath screen: zoom in, wires stop exactly at the card's edge.",
    "Control-signal waveform: traces are cut at the plot's left and right edges as cycles scroll by."],
   [("CO5_Clipping/Clip.h / Clip.cpp", "<code>cohenSutherland(a, b, rect)</code>", "region codes, trivial accept/reject"),
    ("", "<code>liangBarsky(a, b, rect)</code>", "parametric clipping")],
   """
<p><b>Cohen-Sutherland.</b> Each end point gets a 4-bit code TOP BOTTOM RIGHT LEFT:</p>
""" + table(["", "left", "inside x", "right"], [
       ["top", "1001", "1000", "1010"], ["inside y", "0001", "0000", "0010"], ["bottom", "0101", "0100", "0110"]]) + """
""" + ul(["both codes 0000: trivially accept", "code1 AND code2 &ne; 0: both on the same outside side, trivially reject",
          "otherwise move an outside point to the edge it crosses and repeat"]) + """
<p><b>Liang-Barsky.</b> P(t) = a + t(b - a), 0 &le; t &le; 1. For each edge k: <code>p<sub>k</sub> t &le; q<sub>k</sub></code> with
p = (-dx, dx, -dy, dy) and q = (x - xmin, xmax - x, y - ymin, ymax - y). p &lt; 0 means entering:
t0 = max(t0, q/p); p &gt; 0 means leaving: t1 = min(t1, q/p). If t0 &gt; t1 the line is outside.</p>
""",
   [("Which is faster and why?", "Liang-Barsky: it computes the clipped points directly from t, with no repeated intersections."),
    ("What does a non-zero AND of the codes mean?", "Both points lie beyond the same edge, so the whole line is outside."),
    ("p<sub>k</sub> = 0?", "The line is parallel to that edge; if q<sub>k</sub> &lt; 0 it is outside, otherwise that edge is ignored."),
    ("Why clip in software when OpenGL also clips?", "To control exactly which part is drawn (effects along the visible part) and avoid drawing over neighbouring panels."),
    ("Point clipping condition?", "xmin &le; x &le; xmax and ymin &le; y &le; ymax.")])

co("CG", 6, "CO6_TextureLightColor", "Design the texture images, light and color using different objects.",
   "All text is a TrueType glyph atlas texture; the 3D chip's board is a procedurally generated circuit texture with "
   "mipmaps; the chip is lit by two lights (one tinted with the current pipeline phase colour) with specular highlights, "
   "emissive glow on the active unit, and depth fog.",
   ["Every label on screen: textured quads from the font atlas.",
    "3D Chip: textured board, lighting, glow on the unit in use, fog in the distance.",
    "Colour coding: each phase (IF ID EX MEM WB) has its own colour in the datapath, waveform and pipeline."],
   [("CO6_TextureLightColor/Font.h / .cpp", "<code>Font::load</code>, <code>draw</code>, <code>width</code>", "glyph atlas texture (stb_truetype)"),
    ("CO6_TextureLightColor/Texture.h / .cpp", "<code>makePcbTexture</code>", "procedural image + gluBuild2DMipmaps"),
    ("CO6_TextureLightColor/Lighting.h / .cpp", "<code>material</code>, <code>chipLights</code>, <code>depthFog</code>", "Phong lighting and fog")],
   """
<p><b>Text as a texture.</b> At start-up each font size is rasterised into one atlas image, uploaded with
<code>glTexImage2D</code>. Drawing a string is one textured GL_QUAD per character whose texture coordinates select
that glyph from the atlas.</p>
<p><b>Procedural texture.</b> <code>makePcbTexture</code> fills a 512 &times; 512 RGBA image with a grid, noise, copper traces
and vias, then <code>gluBuild2DMipmaps</code> builds the smaller levels. Filtering is <code>GL_LINEAR_MIPMAP_LINEAR</code>
(trilinear), wrapping <code>GL_REPEAT</code>, combined with lighting by <code>GL_MODULATE</code>.</p>
<p><b>Lighting.</b> Colour = emission + ambient + diffuse &middot; (N&middot;L) + specular &middot; (R&middot;V)<sup>shininess</sup>.</p>
""" + table(["Light", "Type", "Purpose"], [
       ["GL_LIGHT0", "directional (w = 0)", "white key light"],
       ["GL_LIGHT1", "point (w = 1), quadratic attenuation", "tinted with the phase colour"]]) + """
<p><b>Fog.</b> <code>GL_LINEAR</code>: f = (end - z) / (end - start); far units blend into the background colour.</p>
""",
   [("Why mipmaps?", "A texture seen far away would shimmer; smaller pre-filtered levels are used instead."),
    ("What does GL_MODULATE do?", "Multiplies the texture colour by the lit surface colour, so the texture is shaded."),
    ("Directional vs point light?", "w = 0 in GL_POSITION means a direction (parallel rays); w = 1 a position (rays spread, attenuation applies)."),
    ("What makes the active unit glow?", "Its material's GL_EMISSION is raised; emission is added regardless of lights."),
    ("Ambient vs diffuse vs specular?", "Ambient: constant base. Diffuse: depends on the angle to the light. Specular: shiny highlight depending on the viewer.")])

# ============================== PSOOP =======================================
co("PSOOP", 1, "CO1_RealWorldOOP", "Develop solutions for real world problems using Object Oriented Programming.",
   "The simulator models a real computer as classes (CPU, ALU, Register64, Assembler, Compiler, ControlUnit, Pipeline) "
   "and solves a real problem for beginners: understanding code written in plain English. The EnglishTranslator "
   "turns sentences like “add 12 and 3” into program statements.",
   ["Compiler screen, English mode: every word is coloured by its role (verb, number, variable, operator, target) and the matched rule is shown."],
   [("CO1_RealWorldOOP/English.h / .cpp", "<code>EnglishTranslator::translate</code>", "English &rarr; statements"),
    ("", "<code>statement()</code>, <code>expr()</code>, <code>term()</code>, <code>condition()</code>, <code>target()</code>", "recursive-descent parser over words"),
    ("core/*", "<code>CPU</code>, <code>ALU</code>, <code>Register64</code>, <code>Assembler</code>, <code>ControlUnit</code>, <code>Pipeline</code>", "hardware as objects")],
   """
<p><b>Objects mirror the machine.</b> A <code>CPU</code> object owns a register file, flags, memory and a program
counter; it executes <code>Instruction</code> objects; a <code>ControlUnit</code> object turns each instruction into
micro-steps; a <code>Pipeline</code> object schedules them. Each class hides its data and offers operations, just as
each hardware unit has inputs and outputs.</p>
<p><b>The English translator</b> is a recursive-descent parser whose tokens are words. Each grammar rule is a member
function and each word is tagged with a <code>WordRole</code>:</p>
""" + code("""
statement := "add" expr "and" expr ["into" target]
           | "set" target "to" expr
           | "if" condition "then" statement ["otherwise" statement]
           | "repeat" number "times" ...    | "find the factorial of" number ...
expr      := term { operator-word term }      ("plus", "times", "minus" ...)
""") + table(["English", "Generated statement"], [
       ["add 12 and 3", "result = 12 + 3"],
       ["if marks is at least 40 then set pass to 1 otherwise set pass to 0",
        "if (marks &gt;= 40) { pass = 1 } else { pass = 0 }"]]),
   [("Why model the CPU as classes?", "Each unit has its own state and behaviour; encapsulation keeps, for example, the register file private to the CPU."),
    ("What is recursive descent?", "One function per grammar rule; rules call each other, following the structure of the sentence."),
    ("How does the translator report errors?", "It throws a SyntaxError with the sentence's line; the UI highlights it and suggests valid verbs."),
    ("What does WordRole give the user?", "Colour-coded feedback showing how each word was understood."),
    ("Real-world benefit?", "Beginners can see the path from plain English to code to machine instructions.")])

co("PSOOP", 2, "CO2_ReduceComplexity", "Choose an appropriate programming solution to reduce complexity.",
   "Complexity is reduced with abstraction and inheritance: the CPU only calls execute() on an abstract Instruction; "
   "control units, hash tables, sorters and screens are abstract base classes with interchangeable subclasses; "
   "containers are templates; and every error is one typed exception hierarchy caught in one place.",
   ["Any error (syntax, divide by zero, stack overflow...) shows as one consistent error toast with the line number."],
   [("CO2_ReduceComplexity/Errors.h / .cpp", "<code>SimulatorError</code> and 6 subclasses", "exception hierarchy"),
    ("core/Instruction.h", "<code>Instruction</code> &rarr; DataTransfer, Arithmetic, Logic, ControlFlow, Stack, System", "polymorphic instructions"),
    ("core/ControlUnit.h", "<code>ControlUnit</code> &rarr; Hardwired, Microprogrammed", "abstract control unit"),
    ("PL CO4 / CO5", "<code>Sorter</code>, <code>Searcher</code>, <code>HashTable</code>; templates <code>Stack&lt;T&gt;</code>, <code>HashMap&lt;K,V&gt;</code>", "abstraction and generic code")],
   """
<p><b>Polymorphism removes switch statements.</b> The CPU does not know about 33 mnemonics:</p>
""" + code("""
ExecRecord rec;
prog_->code[pc_]->execute(*this, rec);   // virtual call: the right subclass runs
""") + """
<p>Adding an instruction means adding one subclass method; the CPU stays unchanged.</p>
<p><b>Exception hierarchy.</b></p>
""" + code("""
std::runtime_error
  +-- SimulatorError          kind(), line()
        +-- SyntaxError          +-- InvalidOpcodeError
        +-- DivideByZeroError    +-- MemoryAccessError
        +-- StackOverflowError   +-- RuntimeLimitError
""") + """
<p>Throwing code says only <i>what</i> went wrong; <code>catch (const SimulatorError&amp; e)</code> in <code>Sim</code>
handles all of them, using <code>e.kind()</code> and <code>e.line()</code>.</p>
<p><b>Templates</b> write a container once for every type (<code>Stack&lt;std::string&gt;</code> for operators,
<code>Stack&lt;int&gt;</code> for free registers). <b>Namespaces</b> (<code>vas::core</code>, <code>vas::ds</code>,
<code>vas::gfx</code>, <code>vas::ui</code>) separate the layers.</p>
""",
   [("Abstract class?", "A class with at least one pure virtual function; it cannot be instantiated and defines an interface."),
    ("Why catch by const reference?", "No copy, and no slicing: the derived type's kind() is kept."),
    ("Templates vs inheritance?", "Templates: same code for different types at compile time. Inheritance: different behaviour behind one interface at run time."),
    ("Why a virtual destructor in base classes?", "Deleting a derived object through a base pointer must run the derived destructor."),
    ("How does this reduce complexity?", "Each part depends only on an interface, so parts can change or be added independently.")])

co("PSOOP", 3, "CO3_ProblemSolving", "Apply critical thinking and programming skills for problem solving.",
   "The central problem of the project: compile statements and simple C++ into x86-64 assembly. The compiler splits it "
   "into three stages (lexer, shunting-yard parser, code generator), each recorded so the screen can animate it.",
   ["Compiler screen, English / C++ modes: token stream, shunting-yard animation, symbol table and the generated assembly.",
    "Parse &amp; Chip &rarr; Parse tree: the expression tree built from the postfix."],
   [("CO3_ProblemSolving/Compiler.h / .cpp", "<code>Compiler::compile</code>", "entry point, translation cache"),
    ("", "<code>lex()</code>", "characters &rarr; tokens (keywords by binary search)"),
    ("", "<code>statement()</code>, <code>ifStatement()</code>, <code>whileStatement()</code>, <code>forStatement()</code>", "statement parsing"),
    ("", "<code>shunt()</code>, <code>genPostfix()</code>, <code>symbol()</code>", "infix &rarr; postfix &rarr; instructions, symbol table")],
   """
<p><b>Breaking the problem down.</b></p>
""" + ul(["<b>Lexer</b>: reads characters, groups them into number, identifier, keyword and operator tokens in a singly linked list.",
          "<b>Parser</b>: statements are parsed by recursive descent; expressions are converted to postfix with the shunting-yard algorithm (operator stack + output queue).",
          "<b>Code generator</b>: walks the postfix with a value stack; each operator takes registers from a free-register stack and emits x86-64; variables get addresses in a hash-table symbol table.",
          "<b>Control flow</b>: if / while / for become CMP + conditional jumps to generated labels."]) + """
<p><b>Other C++ skills used:</b> operator overloading (<code>Register64</code> arithmetic, <code>Vec2</code>,
<code>Mat3</code>), the rule of five in the containers, exceptions for errors, and a <code>HashMap</code> cache so an
unchanged program is not compiled twice.</p>
""",
   [("Why convert to postfix?", "Postfix has no parentheses or precedence; it can be evaluated left to right with one stack."),
    ("How are registers allocated?", "Free registers sit on a stack; an operation pops one for its result and pushes it back when the value is no longer needed."),
    ("How is if/else compiled?", "Compare, jump to the else label if the condition is false, then-part, jump to end, else label, else-part, end label."),
    ("Why record every stage?", "So the UI can animate the process step by step: the purpose of the tool is understanding."),
    ("What does the cache key on?", "The source text and hash method; a hit returns the previous assembly.")],
   "<p><b>Real output</b> for <code>c = a + b * 2</code> (postfix <code>a b 2 * +</code>):</p>" + code("""
section .data
    a         dq 0
    b         dq 0
    c         dq 0
section .text
_start:
    mov rbx, [b]
    imul rbx, 2
    mov rsi, [a]
    add rsi, rbx
    mov [c], rsi
    hlt
"""))

co("PSOOP", 4, "CO4_LogicBuilding", "Utilize logic building traits efficiently for solving real life problems.",
   "Design patterns solve recurring problems in the simulator: a Factory creates instructions from mnemonics, an "
   "Observer profiles execution, a Singleton holds the clock, Strategies make the control unit, hash method and sort "
   "interchangeable, and Command / undo powers Step Back. A friend class and a static member complete the set.",
   ["Top bar: “cycle N” is the Singleton clock; “executed N” is the static member Instruction::executedCount.",
    "Data Lab sorts the Profiler's counts (Observer).", "Ctrl+E exports the trace written by the friend class Debugger.",
    "Step Back undoes instructions (Command / undo)."],
   [("CO4_LogicBuilding/Clock.h / .cpp", "<code>Clock::instance()</code>", "Singleton"),
    ("CO4_LogicBuilding/Profiler.h / .cpp", "<code>CPUObserver</code>, <code>Profiler::onStep</code>", "Observer"),
    ("CO4_LogicBuilding/Debugger.h / .cpp", "<code>dumpRegisters</code>, <code>describe</code>", "friend class of CPU"),
    ("core/Instruction.cpp", "<code>InstructionFactory</code>, <code>executedCount</code>", "Factory, static member"),
    ("app/Sim.cpp", "<code>stepBack()</code>, <code>exportTrace()</code>, <code>saveSource()</code>", "Command / undo, file handling")],
   table(["Pattern", "Problem it solves here", "How"], [
       ["Factory", "create the right Instruction subclass from text", "HashMap mnemonic &rarr; creator function"],
       ["Observer", "count instructions without changing the CPU", "CPU calls onStep() on every registered observer"],
       ["Singleton", "one clock for the whole program", "private constructor, deleted copy, static instance()"],
       ["Strategy", "switch hardwired / micro-programmed CU, hash method, sort", "abstract base + interchangeable subclasses"],
       ["Command / undo", "Step Back", "snapshot before each instruction in a doubly linked list; restore it"],
       ["Friend class", "export private CPU state", "<code>friend class Debugger;</code> in CPU"],
       ["Static member", "count all executions across instructions", "<code>static long executedCount</code> shared by every Instruction"]]) + code("""
Clock& Clock::instance() {
    static Clock c;      // created once, on first use
    return c;
}
"""),
   [("Why is the Clock constructor private?", "So no other code can create a second clock; instance() is the only access."),
    ("Benefit of the Observer?", "The CPU does not depend on the Profiler; observers can be added without changing CPU code."),
    ("Does friend break encapsulation?", "Only for one named class, for a read-only debugging purpose, instead of adding public getters for everything."),
    ("Static vs instance member?", "One copy shared by all objects vs one copy per object."),
    ("Where is file handling used?", "Ctrl+S / Ctrl+O save and load the program; Ctrl+E writes the execution trace.")])


# --------------------------------------------------------------------------
# rendering
# --------------------------------------------------------------------------
CSS = """
@page { size: A4; margin: 16mm 15mm 16mm 15mm; }
body { font-family: 'DejaVu Sans', Arial, sans-serif; font-size: 10.2pt; color: #1b2330; line-height: 1.42; }
.band { background: #1f3a68; color: #fff; padding: 12px 16px; border-radius: 6px; }
.band .sub { font-size: 9pt; opacity: .85; letter-spacing: .04em; text-transform: uppercase; }
.band h1 { margin: 4px 0 2px; font-size: 17pt; }
.band .stmt { font-size: 10.5pt; font-style: italic; opacity: .95; }
.summary { background: #eef3fb; border-left: 4px solid #3b6fc4; padding: 8px 12px; margin: 12px 0; }
h2 { font-size: 12pt; color: #1f3a68; border-bottom: 1px solid #c9d4e6; padding-bottom: 2px; margin: 16px 0 6px; }
table { border-collapse: collapse; width: 100%; margin: 6px 0 8px; font-size: 9.3pt; page-break-inside: avoid; }
th { background: #e3eaf6; text-align: left; }
th, td { border: 1px solid #c9d4e6; padding: 4px 6px; vertical-align: top; }
pre { background: #f5f7fa; border: 1px solid #dde3ec; padding: 7px 9px; font-size: 8.6pt; line-height: 1.3;
      white-space: pre-wrap; page-break-inside: avoid; }
code { font-family: 'DejaVu Sans Mono', monospace; font-size: 8.9pt; }
.q { font-weight: bold; margin-top: 6px; }
.a { margin: 1px 0 4px 14px; }
.foot { margin-top: 14px; font-size: 8pt; color: #7a8698; border-top: 1px solid #dde3ec; padding-top: 4px; }
"""


def render(c):
    folder, subj_name, subj_code = SUBJECTS[c["subject"]]
    files = table(["File", "Functions / classes", "Role"], [list(f) for f in c["files"]])
    viva = "".join(f"<div class='q'>Q. {q}</div><div class='a'>{a}</div>" for q, a in c["viva"])
    example = f"<h2>Worked example</h2>{c['example']}" if c["example"] else ""
    return f"""<!doctype html><html><head><meta charset="utf-8"><style>{CSS}</style></head><body>
<div class="band"><div class="sub">{c['subject']} &middot; {subj_name} &middot; {subj_code}</div>
<h1>{subj_code}.CO{c['num']}</h1><div class="stmt">{html.escape(c['statement'])}</div></div>
<div class="summary">{c['summary']}</div>
<h2>Where you see it in the simulator</h2>{ul(c['where'])}
<h2>Files and functions</h2>
<p>Paths are relative to <code>src/{folder}/</code> unless they start with another top-level folder.</p>{files}
<h2>How it works</h2>{c['how']}
{example}
<h2>Viva questions</h2>{viva}
<div class="foot">Virtual ALU Simulator &middot; {subj_code}.CO{c['num']} &middot; src/{folder}/{c['folder']}/</div>
</body></html>"""


def find_chrome():
    for name in ("google-chrome", "chromium", "chromium-browser", "google-chrome-stable"):
        p = shutil.which(name)
        if p:
            return p
    sys.exit("Chrome / Chromium not found")


def main():
    chrome = find_chrome()
    tmp = tempfile.mkdtemp()
    for c in COS:
        folder = SUBJECTS[c["subject"]][0]
        out_dir = os.path.join(ROOT, "src", folder, c["folder"])
        out_pdf = os.path.join(out_dir, f"CO{c['num']}_Explanation.pdf")
        page = os.path.join(tmp, f"{c['subject']}_CO{c['num']}.html")
        with open(page, "w", encoding="utf-8") as f:
            f.write(render(c))
        subprocess.run([chrome, "--headless=new", "--disable-gpu", "--no-pdf-header-footer",
                        f"--user-data-dir={tmp}/profile", f"--print-to-pdf={out_pdf}", "file://" + page],
                       check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        print("wrote", os.path.relpath(out_pdf, ROOT))
    shutil.rmtree(tmp, ignore_errors=True)


if __name__ == "__main__":
    main()
