#include "PSOOP_2310261L/CO3_ProblemSolving/Compiler.h"

#include <cctype>
#include <cerrno>
#include <climits>
#include <cstdlib>
#include <sstream>

#include "PL_2310214L/CO2_LinkedLists/LinkedList.h"
#include "PL_2310214L/CO3_StackQueue/Queue.h"
#include "PL_2310214L/CO4_SearchingSorting/Searching.h"
#include "PL_2310214L/CO3_StackQueue/Stack.h"
#include "core/CPU.h"
#include "PSOOP_2310261L/CO2_ReduceComplexity/Errors.h"
#include "core/Instruction.h"

namespace vas::core {

const char* tokTypeName(TokType t) {
    switch (t) {
        case TokType::Number: return "NUMBER";
        case TokType::Ident: return "IDENT";
        case TokType::Keyword: return "KEYWORD";
        case TokType::Op: return "OPERATOR";
        case TokType::RelOp: return "RELOP";
        case TokType::Assign: return "ASSIGN";
        case TokType::LParen: return "LPAREN";
        case TokType::RParen: return "RPAREN";
        case TokType::LBrace: return "LBRACE";
        case TokType::RBrace: return "RBRACE";
        case TokType::Semi: return "SEMI";
        case TokType::OpAssign: return "OP-ASSIGN";
        case TokType::IncDec: return "INC/DEC";
        case TokType::Comma: return "COMMA";
        case TokType::String: return "STRING";
        case TokType::End: return "END";
    }
    return "?";
}

const std::vector<std::string>& Compiler::keywordTable() {
    static const std::vector<std::string> k = {"auto", "else", "for", "if", "int", "long", "return", "using", "while"};   // kept sorted
    return k;
}

namespace {

struct PostItem {
    enum Kind { Num, Var, Bin, Un } kind;
    std::string text;
    std::int64_t value = 0;
};

struct Value {
    enum Kind { Imm, Mem, Reg } kind = Imm;
    std::int64_t imm = 0;
    std::string name;
    int reg = -1;
    std::string str(bool sized = false) const {
        if (kind == Imm) return std::to_string(imm);
        if (kind == Mem) return std::string(sized ? "qword " : "") + "[" + name + "]";
        return regName(reg);
    }
};

bool fits32(std::int64_t v) { return v >= INT32_MIN && v <= INT32_MAX; }

int precedence(const std::string& op) {
    if (op == "neg" || op == "~") return 7;
    if (op == "*" || op == "/" || op == "%") return 6;
    if (op == "+" || op == "-") return 5;
    if (op == "<<" || op == ">>") return 4;
    if (op == "&") return 3;
    if (op == "^") return 2;
    if (op == "|") return 1;
    return 0;
}
bool rightAssoc(const std::string& op) { return op == "neg" || op == "~"; }
bool isUnary(const std::string& op) { return op == "neg" || op == "~"; }

const char* inverseJump(const std::string& rel) {
    if (rel == ">") return "jle";
    if (rel == "<") return "jge";
    if (rel == ">=") return "jl";
    if (rel == "<=") return "jg";
    if (rel == "==") return "jne";
    return "je";   // !=
}
const char* relWords(const std::string& rel) {
    if (rel == ">") return "greater than";
    if (rel == "<") return "less than";
    if (rel == ">=") return "greater or equal to";
    if (rel == "<=") return "less or equal to";
    if (rel == "==") return "equal to";
    return "not equal to";
}

class CompilerImpl {
public:
    CompilerImpl(CompileResult& r, ds::HashMethod m) : R(r) {
        R.symtab.reset(ds::makeHashTable(m, 7));
        // temporaries; RAX/RDX are kept for IDIV, RCX for shift counts
        const int temps[] = {R15, R14, R13, R12, R11, R10, R9, R8, RDI, RSI, RBX};
        for (int t : temps) freeRegs_.push(t);
    }

    void run(const std::string& src) {
        src_ = src;
        lex();
        it_ = list_.begin();
        while (cur().type != TokType::End) statement();
        std::vector<int> l = {emit("    hlt")};
        event(TimelineEvent::Emit, l, "End of program: HLT stops the processor");
        R.timeline.push_back({TimelineEvent::Done});
        buildListing();
    }

private:
    // ------------------------------------------------------------- lexer
    void lex() {
        const std::string& s = src_;
        std::size_t i = 0, n = s.size();
        int line = 0, col = 0;
        auto adv = [&](std::size_t k) {
            for (std::size_t q = 0; q < k && i < n; ++q, ++i) {
                if (s[i] == '\n') { ++line; col = 0; } else ++col;
            }
        };
        while (i < n) {
            char c = s[i];
            if (std::isspace(static_cast<unsigned char>(c))) { adv(1); continue; }
            if (c == '#' || (c == '/' && i + 1 < n && s[i + 1] == '/')) {
                while (i < n && s[i] != '\n') adv(1);
                continue;
            }
            Token t;
            t.line = line;
            t.col = col;
            t.start = int(i);
            LexEvent ev;
            if (std::isdigit(static_cast<unsigned char>(c))) {
                std::size_t j = i;
                int base = 10;
                if (c == '0' && j + 1 < n && (s[j + 1] == 'x' || s[j + 1] == 'X')) {
                    base = 16;
                    j += 2;
                    while (j < n && std::isxdigit(static_cast<unsigned char>(s[j]))) ++j;
                } else {
                    while (j < n && std::isdigit(static_cast<unsigned char>(s[j]))) ++j;
                }
                if (j < n && (std::isalpha(static_cast<unsigned char>(s[j])) || s[j] == '_'))
                    throw SyntaxError("Bad number '" + s.substr(i, j - i + 1) + "'", line);
                t.type = TokType::Number;
                t.text = s.substr(i, j - i);
                errno = 0;
                unsigned long long v = std::strtoull(base == 16 ? t.text.c_str() + 2 : t.text.c_str(), nullptr, base);
                if (errno == ERANGE || v > static_cast<unsigned long long>(INT64_MAX))
                    throw SyntaxError("Number " + t.text + " does not fit in 64 bits", line);
                t.value = static_cast<std::int64_t>(v);
                adv(j - i);
            } else if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
                std::size_t j = i;
                while (j < n && (std::isalnum(static_cast<unsigned char>(s[j])) || s[j] == '_')) ++j;
                t.text = s.substr(i, j - i);
                int k = ds::binarySearch(Compiler::keywordTable(), t.text,
                                         [](const std::string& a, const std::string& b) { return a.compare(b); },
                                         &ev.keywordProbes);
                ev.keywordHit = k >= 0;
                t.type = k >= 0 ? TokType::Keyword : TokType::Ident;
                if (t.type == TokType::Ident && Assembler_isReserved(t.text))
                    throw SyntaxError("'" + t.text + "' is a register name - pick another variable name", line);
                adv(j - i);
            } else if (c == '"' || c == '\'') {
                std::size_t j = i + 1;
                while (j < n && s[j] != c && s[j] != '\n') { if (s[j] == '\\') ++j; ++j; }
                if (j >= n || s[j] != c) throw SyntaxError("Unterminated string literal", line);
                t.type = TokType::String;
                t.text = s.substr(i, j - i + 1);
                adv(j - i + 1);
            } else if (c == ':' && i + 1 < n && s[i + 1] == ':') {
                adv(2);   // std::cout  ->  std cout
                continue;
            } else {
                std::string two = i + 1 < n ? s.substr(i, 2) : "";
                std::string three = i + 2 < n ? s.substr(i, 3) : "";
                if (three == "<<=" || three == ">>=") {
                    t.type = TokType::OpAssign; t.text = three; adv(3);
                } else if (two == "+=" || two == "-=" || two == "*=" || two == "/=" || two == "%=" || two == "&=" ||
                           two == "|=" || two == "^=") {
                    t.type = TokType::OpAssign; t.text = two; adv(2);
                } else if (two == "++" || two == "--") {
                    t.type = TokType::IncDec; t.text = two; adv(2);
                } else if (two == "&&" || two == "||") {
                    throw SyntaxError("'" + two + "' is not supported - use a nested if instead", line);
                } else if (two == "==" || two == "!=" || two == "<=" || two == ">=") {
                    t.type = TokType::RelOp; t.text = two; adv(2);
                } else if (two == "<<" || two == ">>") {
                    t.type = TokType::Op; t.text = two; adv(2);
                } else {
                    t.text = std::string(1, c);
                    switch (c) {
                        case '+': case '-': case '*': case '/': case '%':
                        case '&': case '|': case '^': case '~': t.type = TokType::Op; break;
                        case '(': t.type = TokType::LParen; break;
                        case ')': t.type = TokType::RParen; break;
                        case '{': t.type = TokType::LBrace; break;
                        case '}': t.type = TokType::RBrace; break;
                        case ';': t.type = TokType::Semi; break;
                        case ',': t.type = TokType::Comma; break;
                        case '=': t.type = TokType::Assign; break;
                        case '<': case '>': t.type = TokType::RelOp; break;
                        default:
                            throw SyntaxError(std::string("Unexpected character '") + c + "'", line);
                    }
                    adv(1);
                }
            }
            t.end = int(i);
            ev.tok = t;
            R.tokens.push_back(t);
            R.lex.push_back(ev);
            list_.pushBack(t);
            R.timeline.push_back({TimelineEvent::Lex, int(R.lex.size()) - 1});
        }
        Token end;
        end.type = TokType::End;
        end.line = line;
        end.start = end.end = int(n);
        list_.pushBack(end);
        if (R.lex.empty()) throw SyntaxError("Type a statement first, for example:  c = a + b * 2", 0);
    }

    static bool Assembler_isReserved(const std::string& s) {
        for (int r = 0; r < REG_COUNT; ++r) if (s == regName(r)) return true;
        return s == "cl" || s == "rip";
    }

    // ------------------------------------------------------ token cursor
    const Token& cur() const { return *it_; }
    const Token& peek() const {
        auto n = it_;
        if (cur().type == TokType::End) return cur();
        ++n;
        return *n;
    }
    const Token& peekN(int k) const {
        auto n = it_;
        for (int q = 0; q < k && n->type != TokType::End; ++q) ++n;
        return *n;
    }
    void next() { if (cur().type != TokType::End) ++it_; }
    void skipToSemi() {
        while (cur().type != TokType::Semi && cur().type != TokType::End && cur().type != TokType::RBrace) next();
        if (cur().type == TokType::Semi) next();
    }
    static bool isType(const Token& t) {
        return t.type == TokType::Keyword && (t.text == "int" || t.text == "long" || t.text == "auto");
    }
    static Token synth(TokType ty, const std::string& text, const Token& at, std::int64_t v = 0) {
        Token t = at;
        t.type = ty;
        t.text = text;
        t.value = v;
        return t;
    }
    void expect(TokType t, const std::string& what) {
        if (cur().type != t) {
            std::string got = cur().type == TokType::End ? "end of input" : "'" + cur().text + "'";
            throw SyntaxError("Expected " + what + " but found " + got, cur().line);
        }
        next();
    }

    // -------------------------------------------------------- statements
    void statement() {
        const Token& t = cur();
        if (t.type == TokType::Semi) { next(); return; }
        if (t.type == TokType::Keyword) {
            if (t.text == "if") { ifStatement(); return; }
            if (t.text == "while") { whileStatement(); return; }
            if (t.text == "for") { forStatement(); return; }
            if (t.text == "using" || t.text == "return") { skipToSemi(); return; }   // C++ boiler-plate
            if (t.text == "else") throw SyntaxError("'else' without a matching 'if'", t.line);
            if (isType(t) && peek().type == TokType::Ident && peekN(2).type == TokType::LParen) { functionDef(); return; }
            if (isType(t)) { simpleStatement(true); return; }
        }
        if (t.type == TokType::LBrace) { block(); return; }
        if (t.type == TokType::Ident) {
            if (t.text == "cout" || t.text == "cin" || t.text == "printf" || t.text == "std" || t.text == "endl") {
                skipToSemi();   // console I/O is not simulated
                return;
            }
            TokType nt = peek().type;
            if (nt == TokType::Assign || nt == TokType::OpAssign || nt == TokType::IncDec) { simpleStatement(true); return; }
            throw SyntaxError("Expected '=' after '" + t.text + "'  (write e.g.  " + t.text + " = 5)", t.line);
        }
        if (t.type == TokType::IncDec) { simpleStatement(true); return; }
        throw SyntaxError("A statement must start with a variable, 'if', 'while' or 'for' - found '" + t.text + "'", t.line);
    }

    // int main() { ... }  - the body is compiled like any other block
    void functionDef() {
        next();   // type
        Token name = cur();
        next();
        expect(TokType::LParen, "'('");
        while (cur().type != TokType::RParen && cur().type != TokType::End) next();
        expect(TokType::RParen, "')'");
        if (name.text != "main")
            throw SyntaxError("Only main() is supported - write your code inside int main() { ... }", name.line);
        if (cur().type != TokType::LBrace) throw SyntaxError("Expected '{' after main()", name.line);
        block();
    }

    // assignment, compound assignment (x += e), x++ / ++x, or a declaration
    void simpleStatement(bool eatSemi) {
        if (isType(cur())) {
            next();
            while (true) {
                if (cur().type != TokType::Ident) throw SyntaxError("Expected a variable name after the type", cur().line);
                Token name = cur();
                next();
                if (cur().type == TokType::Assign) {
                    next();
                    std::vector<Token> e = collectExpr(false);
                    if (e.empty()) throw SyntaxError("Missing value after '" + name.text + " ='", name.line);
                    emitAssign(name, e, name.start, e.back().end);
                } else {
                    emitAssign(name, {synth(TokType::Number, "0", name)}, name.start, name.end);   // int x;  ->  x = 0
                }
                if (cur().type == TokType::Comma) { next(); continue; }
                break;
            }
        } else if (cur().type == TokType::IncDec) {
            Token op = cur();
            next();
            if (cur().type != TokType::Ident) throw SyntaxError("Expected a variable after '" + op.text + "'", op.line);
            Token name = cur();
            next();
            emitAssign(name, {name, synth(TokType::Op, op.text == "++" ? "+" : "-", op), synth(TokType::Number, "1", op, 1)},
                       op.start, name.end);
        } else {
            Token name = cur();
            next();
            Token op = cur();
            next();
            if (op.type == TokType::IncDec) {
                emitAssign(name, {name, synth(TokType::Op, op.text == "++" ? "+" : "-", op), synth(TokType::Number, "1", op, 1)},
                           name.start, op.end);
            } else {
                std::vector<Token> e = collectExpr(false);
                if (e.empty()) throw SyntaxError("Missing value after '" + name.text + " " + op.text + "'", name.line);
                if (op.type == TokType::OpAssign) {
                    // x op= e   ->   x = x op (e)
                    std::vector<Token> e2 = {name, synth(TokType::Op, op.text.substr(0, op.text.size() - 1), op)};
                    bool paren = e.size() > 1;
                    if (paren) e2.push_back(synth(TokType::LParen, "(", op));
                    e2.insert(e2.end(), e.begin(), e.end());
                    if (paren) e2.push_back(synth(TokType::RParen, ")", op));
                    emitAssign(name, e2, name.start, e.back().end);
                } else {
                    emitAssign(name, e, name.start, e.back().end);
                }
            }
        }
        if (eatSemi && cur().type == TokType::Semi) next();
    }

    void block() {
        if (cur().type == TokType::LBrace) {
            int line = cur().line;
            next();
            while (cur().type != TokType::RBrace) {
                if (cur().type == TokType::End) throw SyntaxError("Missing '}' to close the block", line);
                statement();
            }
            next();
        } else {
            statement();
        }
    }

    std::string sourceText(int a, int b) const {
        std::string s = src_.substr(a, b - a);
        std::string out;
        bool sp = false;
        for (char c : s) {
            if (std::isspace(static_cast<unsigned char>(c))) { sp = true; continue; }
            if (sp && !out.empty()) out += ' ';
            sp = false;
            out += c;
        }
        return out;
    }

    int beginStmt(const std::string& kind, int start, int end, int line) {
        R.stmts.push_back({kind, sourceText(start, end), line, start, end});
        int idx = int(R.stmts.size()) - 1;
        R.timeline.push_back({TimelineEvent::StmtBegin, idx});
        return idx;
    }

    void emitAssign(const Token& name, const std::vector<Token>& e, int start, int end) {
        std::string ctx = sourceText(start, end);
        beginStmt("assign", start, end, name.line);
        line_ = name.line;
        int ei = shunt(e, ctx);
        Value v = genPostfix(ei);
        std::uint64_t addr = symbol(name.text);
        std::vector<int> lines;
        std::ostringstream note;
        if (v.kind == Value::Imm && fits32(v.imm)) {
            lines.push_back(emit("    mov qword [" + name.text + "], " + std::to_string(v.imm)));
            note << "Store the constant straight into " << name.text;
        } else {
            Value r = materialize(v, lines);
            lines.push_back(emit("    mov [" + name.text + "], " + std::string(regName(r.reg))));
            note << "Store " << regName(r.reg) << " into " << name.text << " (address 0x" << std::hex << addr << ")";
            release(r);
        }
        event(TimelineEvent::Emit, lines, note.str());
    }

    struct Cond {
        std::string rel;
        Value lhs, rhs;
    };

    Cond condition(const std::string& kind, int stmtStart) {
        expect(TokType::LParen, "'(' after '" + kind + "'");
        return condParts(kind, stmtStart, TokType::RParen, "')' to close the condition");
    }

    Cond condParts(const std::string& kind, int stmtStart, TokType closing, const std::string& closeWhat) {
        int line = cur().line;
        std::vector<Token> lhs = collectExpr(true);
        if (lhs.empty()) throw SyntaxError("Missing left side of the condition", line);
        if (cur().type != TokType::RelOp)
            throw SyntaxError("A condition needs a comparison such as  a > b  (found '" + cur().text + "')", line);
        std::string rel = cur().text;
        next();
        std::vector<Token> rhs = collectExpr(true);
        if (rhs.empty()) throw SyntaxError("Missing right side of the condition", line);
        int end = cur().end;
        expect(closing, closeWhat);
        beginStmt(kind, stmtStart, end, line);
        line_ = line;
        Cond c;
        c.rel = rel;
        std::string ctx = kind + " (" + sourceText(lhs.front().start, rhs.back().end) + ")";
        c.lhs = genPostfix(shunt(lhs, ctx + "   left side"));
        c.rhs = genPostfix(shunt(rhs, ctx + "   right side"));
        return c;
    }

    // emits  cmp lhs, rhs  +  inverse jump to `falseLabel`; returns jump line
    int compareAndJump(const Cond& c, const std::string& falseLabel) {
        std::vector<int> lines;
        Value l = materialize(c.lhs, lines);
        Value r = c.rhs;
        if (r.kind == Value::Imm && !fits32(r.imm)) r = materialize(r, lines);
        lines.push_back(emit("    cmp " + std::string(regName(l.reg)) + ", " + r.str(r.kind == Value::Mem)));
        release(r);
        release(l);
        event(TimelineEvent::Emit, lines, "CMP subtracts the two values and only sets the flags (ZF, SF, OF, CF)");
        int j = emit(std::string("    ") + inverseJump(c.rel) + " " + falseLabel);
        event(TimelineEvent::Emit, {j}, std::string("If the left side is NOT ") + relWords(c.rel) +
                                            " the right side, jump to " + falseLabel);
        return j;
    }

    void ifStatement() {
        int start = cur().start;
        next();
        int k = ++labels_;
        std::string elseL = "else_" + std::to_string(k), endL = "endif_" + std::to_string(k);
        Cond c = condition("if", start);
        int jumpLine = compareAndJump(c, elseL);
        block();
        if (cur().type == TokType::Keyword && cur().text == "else") {
            int line = cur().line;
            next();
            std::vector<int> l = {emit("    jmp " + endL), emit(elseL + ":")};
            R.stmts.push_back({"else", "else", line, 0, 0});
            event(TimelineEvent::Emit, l, "THEN part done - jump over the ELSE part; ELSE code starts at " + elseL);
            block();
        } else {
            R.code[jumpLine] = std::string("    ") + inverseJump(c.rel) + " " + endL;   // no else: patch target
        }
        event(TimelineEvent::Emit, {emit(endL + ":")}, "End of the if statement");
    }

    void whileStatement() {
        int start = cur().start;
        next();
        int k = ++labels_;
        std::string top = "while_" + std::to_string(k), endL = "endwhile_" + std::to_string(k);
        int topLine = emit(top + ":");
        Cond c = condition("while", start);
        event(TimelineEvent::Emit, {topLine}, "Loop start: the condition is checked here on every iteration");
        compareAndJump(c, endL);
        block();
        std::vector<int> l = {emit("    jmp " + top), emit(endL + ":")};
        event(TimelineEvent::Emit, l, "Jump back to " + top + " to test the condition again");
    }

    // for (init; cond; step) body   ->   init; top: cmp/jump; body; step; jmp top
    void forStatement() {
        int start = cur().start;
        int line = cur().line;
        next();
        expect(TokType::LParen, "'(' after 'for'");
        if (cur().type != TokType::Semi) simpleStatement(false);
        expect(TokType::Semi, "';' after the for-loop initialisation");
        int k = ++labels_;
        std::string top = "for_" + std::to_string(k), endL = "endfor_" + std::to_string(k);
        int topLine = emit(top + ":");
        Cond c = condParts("for", start, TokType::Semi, "';' after the for-loop condition");
        event(TimelineEvent::Emit, {topLine}, "Loop start: the condition is tested before every iteration");
        compareAndJump(c, endL);
        // remember the step, compile the body first, then come back for the step
        auto stepIt = it_;
        int depth = 0;
        while (cur().type != TokType::End && !(cur().type == TokType::RParen && depth == 0)) {
            if (cur().type == TokType::LParen) ++depth;
            if (cur().type == TokType::RParen) --depth;
            next();
        }
        expect(TokType::RParen, "')' to close the for-loop header");
        block();
        auto after = it_;
        it_ = stepIt;
        if (cur().type != TokType::RParen) simpleStatement(false);
        it_ = after;
        std::vector<int> l = {emit("    jmp " + top), emit(endL + ":")};
        event(TimelineEvent::Emit, l, "Step done - jump back to " + top + " to test the condition again");
        (void)line;
    }

    // collects the tokens of one expression; stops at a statement boundary
    std::vector<Token> collectExpr(bool stopAtRelop) {
        std::vector<Token> out;
        int depth = 0;
        auto operandEnd = [](TokType t) { return t == TokType::Number || t == TokType::Ident || t == TokType::RParen; };
        auto operandStart = [](TokType t) { return t == TokType::Number || t == TokType::Ident || t == TokType::LParen; };
        while (true) {
            const Token& t = cur();
            if (t.type == TokType::End || t.type == TokType::Semi || t.type == TokType::LBrace ||
                t.type == TokType::RBrace || t.type == TokType::Keyword || t.type == TokType::Assign ||
                t.type == TokType::OpAssign || t.type == TokType::IncDec || t.type == TokType::Comma ||
                t.type == TokType::String)
                break;
            if (t.type == TokType::RelOp) {
                if (stopAtRelop && depth == 0) break;
                throw SyntaxError("Comparisons like '" + t.text + "' are only allowed inside if (...) / while (...)",
                                  t.line);
            }
            if (t.type == TokType::RParen && depth == 0) break;
            if (depth == 0 && !out.empty() && operandEnd(out.back().type) && operandStart(t.type)) break;
            if (t.type == TokType::LParen) ++depth;
            if (t.type == TokType::RParen) --depth;
            out.push_back(t);
            next();
        }
        if (depth > 0) throw SyntaxError("Missing ')' in expression", out.empty() ? cur().line : out.back().line);
        return out;
    }

    // ------------------------------------------------- shunting-yard [PL CO3]
    int shunt(const std::vector<Token>& e, const std::string& ctx) {
        ExprTrace tr;
        tr.context = ctx;
        for (const auto& t : e) tr.infix.push_back(t.text);
        ds::Stack<std::string> ops;
        ds::CircularQueue<PostItem> out(16);
        std::vector<PostItem> post;
        int ei = int(R.exprs.size());

        auto snapshot = [&](ShuntStep::Action a, const std::string& tok, const std::string& note) {
            ShuntStep s;
            s.action = a;
            s.token = tok;
            s.note = note;
            for (std::size_t i = 0; i < ops.size(); ++i) s.stack.push_back(ops[i]);
            for (std::size_t i = 0; i < out.size(); ++i) s.output.push_back(out.at(i).text);
            tr.shunt.push_back(s);
        };
        auto popToOutput = [&](const std::string& why) {
            std::string op = ops.pop();
            PostItem p{isUnary(op) ? PostItem::Un : PostItem::Bin, op};
            out.enqueue(p);
            snapshot(ShuntStep::PopOp, op, why);
        };

        bool expectOperand = true;
        for (const auto& t : e) {
            if (t.type == TokType::Number || t.type == TokType::Ident) {
                if (!expectOperand) throw SyntaxError("Missing operator before '" + t.text + "'", t.line);
                PostItem p{t.type == TokType::Number ? PostItem::Num : PostItem::Var, t.text, t.value};
                out.enqueue(p);
                snapshot(ShuntStep::Operand, t.text,
                         std::string(t.type == TokType::Number ? "Number " : "Variable ") + t.text +
                             " goes straight to the output queue");
                expectOperand = false;
            } else if (t.type == TokType::Op) {
                std::string op = t.text;
                if (expectOperand) {
                    if (op == "-") op = "neg";
                    else if (op == "+") { snapshot(ShuntStep::Operand, "+", "Unary '+' does nothing - skipped"); continue; }
                    else if (op != "~") throw SyntaxError("Missing value before '" + op + "'", t.line);
                } else if (op == "~") {
                    throw SyntaxError("'~' must come before a value", t.line);
                }
                while (!ops.empty() && ops.top() != "(") {
                    const std::string& top = ops.top();
                    int pt = precedence(top), po = precedence(op);
                    if (pt > po || (pt == po && !rightAssoc(op))) {
                        std::ostringstream why;
                        why << "'" << top << "' (precedence " << pt << ") binds at least as tightly as '" << op
                            << "' (" << po << ") - pop it to the output";
                        popToOutput(why.str());
                    } else {
                        break;
                    }
                }
                ops.push(op);
                snapshot(ShuntStep::PushOp, op,
                         op == "neg" ? "Unary minus is pushed with the highest precedence"
                                     : "Push '" + op + "' onto the operator stack");
                expectOperand = true;
            } else if (t.type == TokType::LParen) {
                if (!expectOperand) throw SyntaxError("Missing operator before '('", t.line);
                ops.push("(");
                snapshot(ShuntStep::PushParen, "(", "'(' is pushed - it shields the operators below it");
            } else if (t.type == TokType::RParen) {
                if (expectOperand) throw SyntaxError("Empty or incomplete '( )'", t.line);
                while (!ops.empty() && ops.top() != "(") popToOutput("')' found - pop operators until '('");
                if (ops.empty()) throw SyntaxError("Unmatched ')'", t.line);
                ops.pop();
                snapshot(ShuntStep::DropParen, ")", "Discard the matching '(' and ')'");
            }
        }
        if (expectOperand) throw SyntaxError("Expression ends with an operator", e.back().line);
        while (!ops.empty()) {
            if (ops.top() == "(") throw SyntaxError("Unmatched '('", e.back().line);
            popToOutput("End of expression - pop the remaining operators");
        }
        while (!out.empty()) post.push_back(out.dequeue());   // the queue is read front to back
        std::string pf;
        for (const auto& p : post) { tr.postfix.push_back(p.text == "neg" ? "u-" : p.text); pf += tr.postfix.back() + " "; }
        ShuntStep done;
        done.action = ShuntStep::Done;
        done.note = "Postfix (Reverse Polish): " + pf;
        done.output = tr.postfix;
        tr.shunt.push_back(done);

        R.exprs.push_back(tr);
        postfix_.push_back(post);
        for (int i = 0; i < int(R.exprs[ei].shunt.size()); ++i) R.timeline.push_back({TimelineEvent::Shunt, ei, i});
        return ei;
    }

    // ----------------------------------------------------- code generation
    Value genPostfix(int ei) {
        ds::Stack<Value> vals;
        const auto& post = postfix_[ei];
        auto snapshotVals = [&]() {
            std::vector<std::string> v;
            for (std::size_t i = 0; i < vals.size(); ++i) v.push_back(vals[i].str());
            return v;
        };
        for (int pi = 0; pi < int(post.size()); ++pi) {
            const PostItem& p = post[pi];
            GenStep g;
            g.postfixIndex = pi;
            g.item = p.text == "neg" ? "u-" : p.text;
            std::vector<int> lines;
            std::ostringstream note;
            if (p.kind == PostItem::Num) {
                Value v;
                v.kind = Value::Imm;
                v.imm = p.value;
                vals.push(v);
                note << "Constant " << p.value << " waits on the value stack (it may become an immediate)";
            } else if (p.kind == PostItem::Var) {
                std::uint64_t addr = symbol(p.text);
                Value v;
                v.kind = Value::Mem;
                v.name = p.text;
                vals.push(v);
                note << "Variable " << p.text << " lives in memory at 0x" << std::hex << addr
                     << " - pushed as operand [" << p.text << "]";
            } else if (p.kind == PostItem::Un) {
                Value v = vals.pop();
                if (v.kind == Value::Imm) {
                    v.imm = p.text == "neg" ? -v.imm : ~v.imm;
                    vals.push(v);
                    note << "Constant folded at compile time: " << v.imm;
                } else {
                    Value r = materialize(v, lines);
                    lines.push_back(emit(std::string("    ") + (p.text == "neg" ? "neg " : "not ") + regName(r.reg)));
                    vals.push(r);
                    note << (p.text == "neg" ? "NEG computes the two's complement of " : "NOT flips every bit of ")
                         << regName(r.reg);
                }
            } else {
                Value right = vals.pop();
                Value left = vals.pop();
                Value l = materialize(left, lines);
                const std::string& op = p.text;
                std::string L = regName(l.reg);
                if (op == "/" || op == "%") {
                    if (right.kind == Value::Imm) right = materialize(right, lines);
                    lines.push_back(emit("    mov rax, " + L));
                    lines.push_back(emit("    cqo"));
                    lines.push_back(emit("    idiv " + right.str(true)));
                    lines.push_back(emit("    mov " + L + (op == "/" ? ", rax" : ", rdx")));
                    note << "IDIV divides RDX:RAX (CQO sign-extends RAX) - quotient in RAX, remainder in RDX; keep the "
                         << (op == "/" ? "quotient" : "remainder");
                } else if (op == "<<" || op == ">>") {
                    const char* m = op == "<<" ? "shl" : "shr";
                    if (right.kind == Value::Imm) {
                        lines.push_back(emit(std::string("    ") + m + " " + L + ", " + std::to_string(right.imm & 63)));
                        note << (op == "<<" ? "Shift left by " : "Shift right by ") << (right.imm & 63)
                             << " = multiply/divide by 2^" << (right.imm & 63);
                    } else {
                        lines.push_back(emit("    mov rcx, " + right.str(right.kind == Value::Mem)));
                        lines.push_back(emit(std::string("    ") + m + " " + L + ", cl"));
                        note << "A variable shift count must be in CL";
                    }
                } else {
                    const char* m = op == "+" ? "add" : op == "-" ? "sub" : op == "*" ? "imul" : op == "&" ? "and"
                                  : op == "|" ? "or" : "xor";
                    if (right.kind == Value::Imm && !fits32(right.imm)) right = materialize(right, lines);
                    lines.push_back(emit(std::string("    ") + m + " " + L + ", " + right.str(right.kind == Value::Mem)));
                    note << left.str() << " " << op << " " << right.str() << "  ->  " << m << " " << L << ", "
                         << right.str();
                }
                release(right);
                vals.push(l);
            }
            g.note = note.str();
            g.lines = lines;
            g.valueStack = snapshotVals();
            R.exprs[ei].gen.push_back(g);
            TimelineEvent ev{TimelineEvent::Gen, ei, int(R.exprs[ei].gen.size()) - 1};
            ev.lines = lines;
            R.timeline.push_back(ev);
        }
        if (vals.size() != 1) throw SyntaxError("Malformed expression", line_);
        return vals.pop();
    }

    Value materialize(const Value& v, std::vector<int>& lines) {
        if (v.kind == Value::Reg) return v;
        if (freeRegs_.empty())
            throw SyntaxError("Expression too complex - ran out of temporary registers", line_);
        Value r;
        r.kind = Value::Reg;
        r.reg = freeRegs_.pop();
        lines.push_back(emit("    mov " + std::string(regName(r.reg)) + ", " + v.str()));
        return r;
    }
    void release(const Value& v) {
        if (v.kind == Value::Reg) freeRegs_.push(v.reg);
    }

    std::uint64_t symbol(const std::string& name) {
        int addr;
        if (R.symtab->find(name, addr)) return std::uint64_t(addr);
        std::uint64_t a = CPU::kDataBase + 8 * vars_.size();
        SymbolEvent se;
        se.name = name;
        se.addr = a;
        R.symtab->insert(name, int(a), &se.trace);
        vars_.push_back(name);
        R.symbols.push_back(se);
        R.timeline.push_back({TimelineEvent::Symbol, int(R.symbols.size()) - 1});
        return a;
    }

    int emit(const std::string& s) {
        R.code.push_back(s);
        return int(R.code.size()) - 1;
    }
    void event(TimelineEvent::Kind k, std::vector<int> lines, const std::string& note) {
        TimelineEvent e{k};
        e.lines = std::move(lines);
        e.note = note;
        R.timeline.push_back(e);
    }

    void buildListing() {
        R.asmLines.clear();
        R.asmLines.push_back("section .data");
        for (const auto& v : vars_) {
            std::string pad = v.size() < 10 ? std::string(10 - v.size(), ' ') : " ";
            R.asmLines.push_back("    " + v + pad + "dq 0");
        }
        R.asmLines.push_back("");
        R.asmLines.push_back("section .text");
        R.asmLines.push_back("global _start");
        R.asmLines.push_back("_start:");
        R.codeOffset = int(R.asmLines.size());
        for (const auto& c : R.code) R.asmLines.push_back(c);
        std::string t;
        for (const auto& l : R.asmLines) t += l + "\n";
        R.asmText = t;
    }

    CompileResult& R;
    std::string src_;
    ds::SinglyLinkedList<Token> list_;
    ds::SinglyLinkedList<Token>::Iterator it_{nullptr};
    ds::Stack<int> freeRegs_;
    std::vector<std::vector<PostItem>> postfix_;
    std::vector<std::string> vars_;
    int labels_ = 0;
    int line_ = 0;
};

} // namespace

CompileResult Compiler::compile(const std::string& source, ds::HashMethod method) {
    CompileResult r;
    std::string key = source + "#" + std::to_string(int(method));
    r.cacheHit = cache_.contains(key);
    try {
        CompilerImpl impl(r, method);
        impl.run(source);
        r.ok = true;
        cache_.put(key, r.asmText);
    } catch (const SimulatorError& e) {
        r.ok = false;
        r.errorKind = e.kind();
        r.error = e.what();
        r.errorLine = e.line();
    }
    return r;
}

} // namespace vas::core
