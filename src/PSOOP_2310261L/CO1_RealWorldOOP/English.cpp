#include "PSOOP_2310261L/CO1_RealWorldOOP/English.h"

#include <cctype>
#include <cstdlib>
#include <initializer_list>
#include <set>

#include "PSOOP_2310261L/CO2_ReduceComplexity/Errors.h"

namespace vas::core {

namespace {

const std::set<std::string>& reserved() {
    static const std::set<std::string> r = {
        "add", "subtract", "minus", "multiply", "divide", "plus", "times", "multiplied", "divided", "over", "mod",
        "modulo", "by", "and", "to", "from", "of", "the", "is", "if", "then", "else", "otherwise", "set",
        "let", "be", "store", "put", "save", "keep", "place", "write", "in", "into", "as", "find", "compute",
        "calculate", "evaluate", "what", "work", "out", "get", "determine", "print", "show", "display", "tell", "sum",
        "product", "difference", "quotient", "remainder", "average", "increase", "decrease", "increment", "decrement",
        "reduce", "lower", "raise", "double", "triple", "halve", "square", "cube", "negate", "clear", "reset", "swap",
        "exchange", "with", "larger", "largest", "bigger", "biggest", "greater", "greatest", "maximum", "max",
        "highest", "smaller", "smallest", "lesser", "least", "minimum", "min", "lowest", "factorial", "power",
        "repeat", "time", "while", "do", "as", "long", "equals", "equal", "same", "than", "more", "fewer", "less",
        "higher", "above", "below", "at", "most", "not", "or", "make", "change", "update", "assign", "give", "copy",
        "move", "initialize", "initialise", "define", "declare", "create", "it", "that", "this", "result", "giving",
        "called", "named", "please", "now", "next", "first", "finally", "also", "numbers", "number", "integers",
        "all", "natural", "whole", "through", "till", "until", "using", "repeated", "successive", "addition",
        "additions", "twice", "thrice", "half", "between", "among", "becomes", "when", "value", "variable"};
    return r;
}

bool numberWord(const std::string& w, long long& v) {
    static const char* units[] = {"zero", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine", "ten",
                                  "eleven", "twelve", "thirteen", "fourteen", "fifteen", "sixteen", "seventeen",
                                  "eighteen", "nineteen"};
    static const char* tens[] = {"", "", "twenty", "thirty", "forty", "fifty", "sixty", "seventy", "eighty", "ninety"};
    for (int i = 0; i < 20; ++i) if (w == units[i]) { v = i; return true; }
    for (int i = 2; i < 10; ++i) if (w == tens[i]) { v = i * 10; return true; }
    if (w == "hundred") { v = 100; return true; }
    if (w == "thousand") { v = 1000; return true; }
    return false;
}

bool isDigits(const std::string& w) {
    if (w.empty()) return false;
    for (char c : w) if (!std::isdigit(static_cast<unsigned char>(c))) return false;
    return true;
}

bool isIdentifier(const std::string& w) {
    if (w.empty() || !(std::isalpha(static_cast<unsigned char>(w[0])) || w[0] == '_')) return false;
    for (char c : w) if (!(std::isalnum(static_cast<unsigned char>(c)) || c == '_')) return false;
    return true;
}

std::vector<std::string> indent(const std::vector<std::string>& lines) {
    std::vector<std::string> out;
    for (const auto& l : lines) out.push_back("    " + l);
    return out;
}

void append(std::vector<std::string>& a, const std::vector<std::string>& b) { a.insert(a.end(), b.begin(), b.end()); }

std::vector<std::string> tokenize(const std::string& s) {
    std::vector<std::string> out;
    std::size_t i = 0;
    while (i < s.size()) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (std::isspace(c) || c == ',' || c == ':' || c == '?' || c == '\'' || c == '"') { ++i; continue; }
        if (std::isalnum(c) || c == '_') {
            std::size_t j = i;
            while (j < s.size() && (std::isalnum(static_cast<unsigned char>(s[j])) || s[j] == '_')) ++j;
            std::string w = s.substr(i, j - i);
            for (auto& ch : w) ch = char(std::tolower(static_cast<unsigned char>(ch)));
            out.push_back(w);
            i = j;
            continue;
        }
        std::string two = s.substr(i, 2);
        if (two == ">=" || two == "<=" || two == "==" || two == "!=") { out.push_back(two); i += 2; continue; }
        if (std::string("+-*/%=<>()").find(char(c)) != std::string::npos) { out.push_back(std::string(1, char(c))); ++i; continue; }
        ++i;   // other punctuation is ignored
    }
    return out;
}

class Parser {
public:
    Parser(std::vector<std::string> words, int line, std::set<std::string>& used)
        : w_(std::move(words)), roles_(w_.size(), WordRole::Filler), line_(line), used_(used) {}

    std::vector<std::string> sentence() {
        std::vector<std::string> lines = body({});
        if (!done()) {
            std::string rest;
            for (std::size_t i = p_; i < w_.size(); ++i) rest += (rest.empty() ? "" : " ") + w_[i];
            fail("I understood the start of the sentence but not \"" + rest + "\"");
        }
        return lines;
    }
    const std::vector<WordRole>& roles() const { return roles_; }
    const std::vector<std::string>& words() const { return w_; }
    std::string rule;

private:
    // ------------------------------------------------------------ cursor
    bool done() const { return p_ >= w_.size(); }
    const std::string& cur() const {
        static const std::string empty;
        return done() ? empty : w_[p_];
    }
    bool at(const char* s, std::size_t k = 0) const { return p_ + k < w_.size() && w_[p_ + k] == s; }
    bool accept(const char* s, WordRole r = WordRole::Keyword) {
        if (!at(s)) return false;
        roles_[p_++] = r;
        return true;
    }
    bool acceptAny(std::initializer_list<const char*> l, WordRole r = WordRole::Keyword) {
        for (const char* s : l) if (accept(s, r)) return true;
        return false;
    }
    bool atAny(std::initializer_list<const char*> l) const {
        for (const char* s : l) if (at(s)) return true;
        return false;
    }
    void expectAny(std::initializer_list<const char*> l, const std::string& hint) {
        if (!acceptAny(l)) fail("Expected " + hint + (done() ? " at the end" : " before \"" + cur() + "\""));
    }
    [[noreturn]] void fail(const std::string& m) const { throw SyntaxError(m, line_); }
    void restore(std::size_t save) {
        for (std::size_t i = save; i < p_; ++i) roles_[i] = WordRole::Filler;
        p_ = save;
    }

    // ----------------------------------------------------------- operands
    bool numberAt() const {
        long long v;
        return !done() && (isDigits(cur()) || numberWord(cur(), v));
    }
    std::string number() {
        if (isDigits(cur())) { roles_[p_] = WordRole::Number; return w_[p_++]; }
        long long total = 0, group = 0, v;
        bool any = false;
        while (!done() && numberWord(cur(), v)) {
            if (v == 100) group = (group ? group : 1) * 100;
            else if (v == 1000) { total += (group ? group : 1) * 1000; group = 0; }
            else group += v;
            roles_[p_++] = WordRole::Number;
            any = true;
        }
        if (!any) fail("Expected a number");
        return std::to_string(total + group);
    }
    bool varAt(std::size_t k = 0) const {
        if (p_ + k >= w_.size()) return false;
        const std::string& s = w_[p_ + k];
        long long v;
        return isIdentifier(s) && !reserved().count(s) && !numberWord(s, v);
    }
    std::string var(WordRole r = WordRole::Variable) {
        if (!varAt()) {
            if (done()) fail("Expected a variable name at the end of the sentence");
            fail("Expected a variable name but found \"" + cur() + "\"" +
                 (reserved().count(cur()) ? " (it is a keyword)" : ""));
        }
        used_.insert(cur());
        roles_[p_] = r;
        return w_[p_++];
    }
    std::string fresh(const std::string& base) {
        std::string n = base;
        for (int i = 2; used_.count(n); ++i) n = base + std::to_string(i);
        used_.insert(n);
        return n;
    }

    // -------------------------------------------------------- expressions
    std::string opWord() {
        if (acceptAny({"plus", "+"}, WordRole::Operator)) return "+";
        if (acceptAny({"minus", "-"}, WordRole::Operator)) return "-";
        if (acceptAny({"times", "*"}, WordRole::Operator)) return "*";
        if (at("multiplied") && at("by", 1)) { accept("multiplied", WordRole::Operator); accept("by", WordRole::Operator); return "*"; }
        if (at("divided") && at("by", 1)) { accept("divided", WordRole::Operator); accept("by", WordRole::Operator); return "/"; }
        if (acceptAny({"over", "/"}, WordRole::Operator)) return "/";
        if (acceptAny({"mod", "modulo", "%"}, WordRole::Operator)) return "%";
        return "";
    }
    std::string expr() {
        std::string s = term();
        while (true) {
            std::string op = opWord();
            if (op.empty()) break;
            s += " " + op + " " + term();
        }
        return s;
    }
    std::string pair(const char* op) {
        accept("of");
        accept("between");
        std::string a = expr();
        expectAny({"and", "with"}, "\"and\"");
        std::string b = expr();
        return std::string("(") + a + " " + op + " " + b + ")";
    }
    std::string term() {
        if (accept("(", WordRole::Operator)) {
            std::string e = expr();
            expectAny({")"}, "')'");
            roles_[p_ - 1] = WordRole::Operator;
            return "(" + e + ")";
        }
        if (acceptAny({"-", "minus", "negative"}, WordRole::Operator)) {
            std::string t = term();
            return isIdentifier(t) || isDigits(t) ? "-" + t : "-(" + t + ")";
        }
        accept("the", WordRole::Filler);
        if (accept("sum", WordRole::Operator)) return pair("+");
        if (accept("product", WordRole::Operator)) return pair("*");
        if (accept("difference", WordRole::Operator)) return pair("-");
        if (accept("quotient", WordRole::Operator)) return pair("/");
        if (accept("remainder", WordRole::Operator)) {
            accept("of");
            accept("when");
            std::string a = term();
            accept("is");
            if (acceptAny({"divided", "and", "mod"}, WordRole::Operator)) accept("by", WordRole::Operator);
            std::string b = term();
            return "(" + a + " % " + b + ")";
        }
        if (accept("average", WordRole::Operator)) {
            std::string pr = pair("+");
            return "(" + pr + " / 2)";
        }
        if (accept("square", WordRole::Operator)) { accept("of"); std::string a = term(); return "(" + a + " * " + a + ")"; }
        if (accept("cube", WordRole::Operator)) { accept("of"); std::string a = term(); return "(" + a + " * " + a + " * " + a + ")"; }
        if (accept("twice", WordRole::Operator)) return "(2 * " + term() + ")";
        if (accept("thrice", WordRole::Operator)) return "(3 * " + term() + ")";
        if (accept("half", WordRole::Operator)) { accept("of"); return "(" + term() + " / 2)"; }
        if (numberAt()) return number();
        if (varAt()) return var();
        if (done()) fail("The sentence ended where a number or variable was expected");
        fail("Expected a number or a variable but found \"" + cur() + "\"");
    }

    // optional "and store it in x" / "into x" / "as x"
    std::string target() {
        std::size_t save = p_;
        accept("and");
        accept("then");
        if (acceptAny({"store", "save", "put", "keep", "place", "write"}, WordRole::Verb)) {
            acceptAny({"it", "that", "this", "the"});
            accept("result");
            acceptAny({"in", "into", "to", "as"});
            if (varAt()) return var(WordRole::Target);
            restore(save);
            return "";
        }
        if (acceptAny({"into", "in", "as", "giving", "called", "named"}) && varAt()) return var(WordRole::Target);
        restore(save);
        return "";
    }

    std::string condition() {
        std::string l = expr();
        std::string op;
        bool is = accept("is");
        if (accept("not")) {
            acceptAny({"equal", "equals", "same"});
            acceptAny({"to", "as"});
            op = "!=";
        } else if (acceptAny({"greater", "more", "bigger", "larger", "higher", "above"})) {
            accept("than");
            op = ">";
            if (accept("or")) { acceptAny({"equal", "equals"}); accept("to"); op = ">="; }
        } else if (acceptAny({"less", "smaller", "fewer", "lower", "below"})) {
            accept("than");
            op = "<";
            if (accept("or")) { acceptAny({"equal", "equals"}); accept("to"); op = "<="; }
        } else if (at("at") && at("least", 1)) {
            accept("at"); accept("least"); op = ">=";
        } else if (at("at") && at("most", 1)) {
            accept("at"); accept("most"); op = "<=";
        } else if (acceptAny({"equal", "equals", "same"})) {
            acceptAny({"to", "as"});
            op = "==";
        } else if (acceptAny({">", "<", ">=", "<=", "==", "!=", "="}, WordRole::Operator)) {
            op = w_[p_ - 1] == "=" ? "==" : w_[p_ - 1];
        } else if (is) {
            op = "==";
        } else {
            fail("Expected a comparison such as \"is greater than\", \"is less than\" or \"equals\"");
        }
        std::string r = expr();
        return l + " " + op + " " + r;
    }

    // statements joined by "and" / "and then" / "then"
    std::vector<std::string> body(std::initializer_list<const char*> stops) {
        std::vector<std::string> lines;
        while (true) {
            append(lines, statement());
            if (done()) break;
            bool stop = false;
            for (const char* s : stops) if (at(s)) stop = true;
            if (stop) break;
            std::size_t save = p_;
            if (accept("and")) { accept("then"); if (done()) { restore(save); break; } continue; }
            if (accept("then")) continue;
            break;
        }
        return lines;
    }

    std::vector<std::string> counterLoop(const std::string& n, const std::vector<std::string>& inner) {
        std::string k = fresh("k");
        std::vector<std::string> out = {k + " = 0", "while (" + k + " < " + n + ") {"};
        append(out, indent(inner));
        out.push_back("    " + k + " = " + k + " + 1");
        out.push_back("}");
        return out;
    }

    // ----------------------------------------------------------- patterns
    std::vector<std::string> statement() {
        while (acceptAny({"please", "now", "next", "first", "finally", "also", "then"}, WordRole::Filler)) {}
        if (done()) fail("The sentence is empty");
        std::string v = cur();
        auto verb = [&]() { roles_[p_++] = WordRole::Verb; };
        auto inPlace = [](const std::string& s) { return isIdentifier(s); };

        if (v == "add") {
            verb();
            rule = "ADD";
            std::string a = expr();
            if (acceptAny({"and", "with", "plus"})) {
                std::string b = expr();
                std::string t = target();
                return {(t.empty() ? use("result") : t) + " = " + a + " + " + b};
            }
            expectAny({"to"}, "\"and\" or \"to\"");
            std::string b = expr();
            std::string t = target();
            if (t.empty() && inPlace(b)) return {b + " = " + b + " + " + a};
            return {(t.empty() ? use("result") : t) + " = " + b + " + " + a};
        }
        if (v == "subtract" || v == "minus") {
            verb();
            rule = "SUBTRACT";
            std::string a = expr();
            if (accept("from")) {
                std::string b = expr();
                std::string t = target();
                if (t.empty() && inPlace(b)) return {b + " = " + b + " - " + a};
                return {(t.empty() ? use("result") : t) + " = " + b + " - " + a};
            }
            expectAny({"and", "minus"}, "\"from\" or \"and\"");
            std::string b = expr();
            std::string t = target();
            return {(t.empty() ? use("result") : t) + " = " + a + " - " + b};
        }
        if (v == "multiply" || v == "divide") {
            verb();
            bool mul = v == "multiply";
            rule = mul ? "MULTIPLY" : "DIVIDE";
            std::string a = term();
            bool by = accept("by");
            if (!by) expectAny({"and", "with", "times"}, "\"by\" or \"and\"");
            std::string b = term();
            if (mul && accept("using")) {
                acceptAny({"repeated", "successive"});
                acceptAny({"addition", "additions"});
                rule = "MULTIPLY BY REPEATED ADDITION";
                std::string t = target();
                std::string pr = t.empty() ? use("product") : t;
                std::vector<std::string> out = {pr + " = 0"};
                append(out, counterLoop(b, {pr + " = " + pr + " + " + a}));
                return out;
            }
            std::string t = target();
            std::string op = mul ? " * " : " / ";
            if (t.empty() && by && inPlace(a)) return {a + " = " + a + op + b};
            return {(t.empty() ? use("result") : t) + " = " + a + op + b};
        }
        if (v == "if") {
            verb();
            rule = "IF / OTHERWISE";
            std::string c = condition();
            acceptAny({"then", "do"});
            std::vector<std::string> thenB = body({"otherwise", "else"});
            std::vector<std::string> out = {"if (" + c + ") {"};
            append(out, indent(thenB));
            out.push_back("}");
            if (acceptAny({"otherwise", "else"}, WordRole::Verb)) {
                accept("then");
                std::vector<std::string> elseB = body({});
                out.back() = "} else {";
                append(out, indent(elseB));
                out.push_back("}");
            }
            return out;
        }
        if (v == "repeat" && !at("while", 1)) {
            verb();
            rule = "REPEAT N TIMES";
            std::string n = term();
            acceptAny({"times", "time"});
            return counterLoop(n, body({}));
        }
        if (v == "while" || (v == "repeat" && at("while", 1)) || (v == "as" && at("long", 1))) {
            verb();
            if (v == "repeat") accept("while", WordRole::Verb);
            if (v == "as") { accept("long", WordRole::Verb); accept("as", WordRole::Verb); }
            rule = "WHILE LOOP";
            std::string c = condition();
            acceptAny({"do", "then", "repeat"});
            std::vector<std::string> out = {"while (" + c + ") {"};
            append(out, indent(body({})));
            out.push_back("}");
            return out;
        }
        if (v == "find" || v == "compute" || v == "calculate" || v == "evaluate" || v == "what" || v == "work" ||
            v == "get" || v == "determine" || v == "print" || v == "show" || v == "display" || v == "tell") {
            verb();
            if (v == "what") accept("is", WordRole::Verb);
            if (v == "work") accept("out", WordRole::Verb);
            accept("the", WordRole::Filler);
            if (acceptAny({"larger", "largest", "bigger", "biggest", "greater", "greatest", "maximum", "max", "highest"}, WordRole::Operator))
                return maxMin(">", "max");
            if (acceptAny({"smaller", "smallest", "lesser", "least", "minimum", "min", "lowest"}, WordRole::Operator))
                return maxMin("<", "min");
            if (accept("factorial", WordRole::Operator)) {
                rule = "FACTORIAL (LOOP)";
                accept("of");
                std::string n = term();
                std::string t = target();
                std::string f = t.empty() ? use("factorial") : t;
                std::string k = fresh("k");
                return {f + " = 1", k + " = " + n, "while (" + k + " > 1) {", "    " + f + " = " + f + " * " + k,
                        "    " + k + " = " + k + " - 1", "}"};
            }
            if (at("sum") && sentenceHas("from")) {
                accept("sum", WordRole::Operator);
                rule = "SUM OF A RANGE (LOOP)";
                while (acceptAny({"of", "the", "all", "numbers", "number", "integers", "natural", "whole"}, WordRole::Filler)) {}
                expectAny({"from"}, "\"from\"");
                std::string a = term();
                expectAny({"to", "through", "till", "until"}, "\"to\"");
                std::string b = term();
                std::string t = target();
                std::string s = t.empty() ? use("sum") : t;
                std::string k = fresh("k");
                return {s + " = 0", k + " = " + a, "while (" + k + " <= " + b + ") {", "    " + s + " = " + s + " + " + k,
                        "    " + k + " = " + k + " + 1", "}"};
            }
            if (sentenceHas("power")) return power();
            rule = "EVALUATE EXPRESSION";
            std::string e = expr();
            std::string t = target();
            return {(t.empty() ? use("result") : t) + " = " + e};
        }
        if (v == "raise" && sentenceHas("power")) { verb(); return power(); }
        if (v == "increase" || v == "increment" || v == "raise") {
            verb();
            rule = "INCREASE";
            std::string x = var();
            std::string n = accept("by") ? expr() : "1";
            return {x + " = " + x + " + " + n};
        }
        if (v == "decrease" || v == "decrement" || v == "reduce" || v == "lower") {
            verb();
            rule = "DECREASE";
            std::string x = var();
            std::string n = accept("by") ? expr() : "1";
            return {x + " = " + x + " - " + n};
        }
        if (v == "double" || v == "triple" || v == "halve" || v == "square" || v == "cube" || v == "negate" ||
            v == "clear" || v == "reset") {
            verb();
            rule = "UPDATE IN PLACE";
            std::string x = var();
            if (v == "double") return {x + " = " + x + " * 2"};
            if (v == "triple") return {x + " = " + x + " * 3"};
            if (v == "halve") return {x + " = " + x + " / 2"};
            if (v == "square") return {x + " = " + x + " * " + x};
            if (v == "cube") return {x + " = " + x + " * " + x + " * " + x};
            if (v == "negate") return {x + " = -" + x};
            return {x + " = 0"};
        }
        if (v == "swap" || v == "exchange") {
            verb();
            rule = "SWAP";
            std::string a = var();
            expectAny({"and", "with"}, "\"and\"");
            std::string b = var();
            std::string t = fresh("temp");
            return {t + " = " + a, a + " = " + b, b + " = " + t};
        }
        if (v == "set" || v == "make" || v == "change" || v == "update" || v == "let" || v == "initialize" ||
            v == "initialise" || v == "define" || v == "declare" || v == "create") {
            verb();
            rule = "ASSIGN";
            accept("variable", WordRole::Filler);
            std::string x = var(WordRole::Target);
            if (done() || atAny({"and", "then"})) return {x + " = 0"};
            while (acceptAny({"to", "equal", "equals", "=", "as", "be", "becomes"})) {}
            return {x + " = " + expr()};
        }
        if (v == "assign" || v == "give") {
            verb();
            rule = "ASSIGN";
            std::string e = expr();
            expectAny({"to"}, "\"to\"");
            return {var(WordRole::Target) + " = " + e};
        }
        if (v == "store" || v == "put" || v == "save" || v == "copy" || v == "move" || v == "place" || v == "keep" ||
            v == "write") {
            verb();
            rule = "ASSIGN";
            std::string e = expr();
            expectAny({"in", "into", "to", "as"}, "\"in\" or \"into\"");
            return {var(WordRole::Target) + " = " + e};
        }
        if (varAt() && (at("is", 1) || at("equals", 1) || at("=", 1) || at("becomes", 1))) {
            rule = "ASSIGN";
            std::string x = var(WordRole::Target);
            acceptAny({"is", "equals", "=", "becomes"});
            return {x + " = " + expr()};
        }
        fail("I don't know how to \"" + v +
             "\". Start a sentence with: add, subtract, multiply, divide, set, increase, decrease, double, swap, "
             "find, if, repeat or while");
    }

    std::vector<std::string> maxMin(const char* op, const char* def) {
        rule = std::string(op) == ">" ? "LARGEST OF (IF CHAIN)" : "SMALLEST OF (IF CHAIN)";
        acceptAny({"of", "among", "between"});
        std::vector<std::string> items = {term()};
        while (true) {
            std::size_t save = p_;
            if (!acceptAny({"and", "or"})) break;
            if (atAny({"store", "save", "put", "keep", "into", "in", "as", "then"})) { restore(save); break; }
            items.push_back(term());
        }
        std::string t = target();
        std::string m = t.empty() ? use(def) : t;
        std::vector<std::string> out = {m + " = " + items[0]};
        for (std::size_t i = 1; i < items.size(); ++i) {
            out.push_back("if (" + items[i] + " " + op + " " + m + ") {");
            out.push_back("    " + m + " = " + items[i]);
            out.push_back("}");
        }
        return out;
    }

    std::vector<std::string> power() {
        rule = "POWER (LOOP)";
        std::string a = term();
        acceptAny({"to", "raised"});
        accept("to");
        accept("the", WordRole::Filler);
        expectAny({"power"}, "\"power\"");
        roles_[p_ - 1] = WordRole::Operator;
        accept("of");
        std::string n = term();
        std::string t = target();
        std::string pw = t.empty() ? use("power") : t;
        std::vector<std::string> out = {pw + " = 1"};
        append(out, counterLoop(n, {pw + " = " + pw + " * " + a}));
        return out;
    }

    bool sentenceHas(const char* word) const {
        for (std::size_t i = p_; i < w_.size(); ++i) if (w_[i] == word) return true;
        return false;
    }
    std::string use(const std::string& name) {
        used_.insert(name);
        return name;
    }

    std::vector<std::string> w_;
    std::vector<WordRole> roles_;
    std::size_t p_ = 0;
    int line_;
    std::set<std::string>& used_;
};

} // namespace

EnglishResult EnglishTranslator::translate(const std::string& source) {
    EnglishResult res;
    // split into sentences: new lines, ';' and '.' (but not a decimal point)
    struct Raw { std::string text; int line; };
    std::vector<Raw> raws;
    std::string cur;
    int line = 0, startLine = 0;
    auto flush = [&]() {
        std::size_t a = cur.find_first_not_of(" \t\r");
        if (a != std::string::npos) {
            std::string t = cur.substr(a);
            while (!t.empty() && std::isspace(static_cast<unsigned char>(t.back()))) t.pop_back();
            if (!t.empty() && t[0] != '#' && t.rfind("//", 0) != 0) raws.push_back({t, startLine});
        }
        cur.clear();
        startLine = line;
    };
    for (std::size_t i = 0; i < source.size(); ++i) {
        char c = source[i];
        bool decimal = c == '.' && i > 0 && i + 1 < source.size() && std::isdigit(static_cast<unsigned char>(source[i - 1])) &&
                       std::isdigit(static_cast<unsigned char>(source[i + 1]));
        if (c == '\n') { flush(); ++line; startLine = line; continue; }
        if ((c == '.' && !decimal) || c == ';') { flush(); continue; }
        cur += c;
    }
    flush();

    std::set<std::string> used;
    try {
        for (const auto& r : raws) {
            std::vector<std::string> words = tokenize(r.text);
            if (words.empty()) continue;
            Parser p(words, r.line, used);
            EnglishSentence s;
            s.text = r.text;
            s.line = r.line;
            s.code = p.sentence();
            s.rule = p.rule;
            for (std::size_t i = 0; i < p.words().size(); ++i) s.words.push_back({p.words()[i], p.roles()[i]});
            for (const auto& l : s.code) res.code += l + "\n";
            res.sentences.push_back(s);
        }
        if (res.sentences.empty()) throw SyntaxError("Type a sentence first, for example:  add 12 and 3", 0);
    } catch (const SimulatorError& e) {
        res.ok = false;
        res.error = e.what();
        res.errorLine = e.line();
    }
    return res;
}

} // namespace vas::core
