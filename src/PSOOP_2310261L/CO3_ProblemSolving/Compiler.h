// =====================================================================
//  Statement -> x86-64 compiler                  [PSOOP CO3, CO4]
//  The heart of the project: a statement such as
//         c = a + b * 2
//         if (a > b) max = a  else max = b
//         while (i < n) { s = s + i  i = i + 1 }
//  is turned into assembly in three recorded stages so the UI can
//  animate them:
//    1. Lexer   - characters -> tokens (SinglyLinkedList); keywords are
//                 found by binary search in a sorted table   [PL CO2, CO4]
//    2. Parser  - shunting-yard turns infix into postfix with an operator
//                 Stack and an output CircularQueue            [PL CO3]
//    3. CodeGen - postfix -> instructions with a register Stack; the
//                 variable symbol table is a HashTable whose probes are
//                 recorded for the animation                   [PL CO5]
// =====================================================================
#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "PL_2310214L/CO5_Hashing/HashMap.h"
#include "PL_2310214L/CO5_Hashing/HashTable.h"

namespace vas::core {

enum class TokType { Number, Ident, Keyword, Op, RelOp, Assign, LParen, RParen, LBrace, RBrace, Semi, OpAssign, IncDec, Comma, String, End };
const char* tokTypeName(TokType t);

struct Token {
    TokType type = TokType::End;
    std::string text;
    int line = 0, col = 0;     // 0-based
    int start = 0, end = 0;    // byte offsets in the source
    std::int64_t value = 0;
};

struct LexEvent {
    Token tok;
    std::vector<int> keywordProbes;   // binary-search probes into keywordTable()
    bool keywordHit = false;
};

struct ShuntStep {
    enum Action { Operand, PushOp, PopOp, PushParen, DropParen, Flush, Done } action;
    std::string token;
    std::string note;
    std::vector<std::string> stack;    // operator stack after the step (bottom first)
    std::vector<std::string> output;   // output queue after the step
};

struct GenStep {
    int postfixIndex = -1;
    std::string item;
    std::string note;
    std::vector<int> lines;            // code lines emitted by this step
    std::vector<std::string> valueStack;
};

struct ExprTrace {
    std::string context;               // e.g. "c = a + b * 2"
    std::vector<std::string> infix;
    std::vector<std::string> postfix;
    std::vector<ShuntStep> shunt;
    std::vector<GenStep> gen;
};

struct SymbolEvent {
    std::string name;
    std::uint64_t addr = 0;
    ds::ProbeTrace trace;
};

struct StmtTrace {
    std::string kind;                  // assign / if / while / else
    std::string source;
    int line = 0;
    int start = 0, end = 0;            // source offsets
};

struct TimelineEvent {
    enum Kind { Lex, StmtBegin, Shunt, Gen, Symbol, Emit, Done } kind;
    int a = -1, b = -1;
    std::vector<int> lines;
    std::string note;
};

struct CompileResult {
    bool ok = false;
    std::string errorKind, error;
    int errorLine = -1;

    std::vector<Token> tokens;
    std::vector<LexEvent> lex;
    std::vector<StmtTrace> stmts;
    std::vector<ExprTrace> exprs;
    std::vector<SymbolEvent> symbols;
    std::vector<TimelineEvent> timeline;

    std::vector<std::string> code;     // code lines (inside _start)
    std::vector<std::string> asmLines; // full program text, line by line
    int codeOffset = 0;                // asmLines index of code[0]
    std::string asmText;
    bool cacheHit = false;
    std::shared_ptr<ds::HashTable> symtab;
};

class Compiler {
public:
    CompileResult compile(const std::string& source, ds::HashMethod symbolMethod);
    static const std::vector<std::string>& keywordTable();   // sorted

private:
    ds::HashMap<std::string, std::string> cache_;            // translation cache
};

} // namespace vas::core
