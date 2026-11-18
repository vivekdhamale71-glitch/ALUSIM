#include "core/Assembler.h"

#include <cctype>
#include <cstdlib>
#include <sstream>

#include "engine/ds/HashMap.h"
#include "core/CPU.h"
#include "engine/lang/Errors.h"

namespace vas::core {

namespace {

std::string trim(const std::string& s) {
    std::size_t a = 0, b = s.size();
    while (a < b && std::isspace(static_cast<unsigned char>(s[a]))) ++a;
    while (b > a && std::isspace(static_cast<unsigned char>(s[b - 1]))) --b;
    return s.substr(a, b - a);
}
std::string lower(std::string s) {
    for (auto& c : s) c = char(std::tolower(static_cast<unsigned char>(c)));
    return s;
}
bool isIdentStart(char c) { return std::isalpha(static_cast<unsigned char>(c)) || c == '_' || c == '.'; }
bool isIdentChar(char c) { return std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '.'; }
bool isIdent(const std::string& s) {
    if (s.empty() || !isIdentStart(s[0])) return false;
    for (char c : s) if (!isIdentChar(c)) return false;
    return true;
}

bool parseNumber(const std::string& sIn, std::int64_t& out) {
    std::string s = lower(trim(sIn));
    if (s.empty()) return false;
    bool neg = false;
    if (s[0] == '-' || s[0] == '+') { neg = s[0] == '-'; s = trim(s.substr(1)); }
    if (s.empty()) return false;
    int base = 10;
    if (s.size() > 2 && s[0] == '0' && s[1] == 'x') { base = 16; s = s.substr(2); }
    else if (s.size() > 1 && s.back() == 'h' && std::isdigit(static_cast<unsigned char>(s[0]))) { base = 16; s.pop_back(); }
    else if (s.size() > 1 && s.back() == 'b' && (s[0] == '0' || s[0] == '1')) {
        bool bin = true;
        for (std::size_t i = 0; i + 1 < s.size(); ++i) if (s[i] != '0' && s[i] != '1') bin = false;
        if (bin) { base = 2; s.pop_back(); }
    }
    if (s.empty()) return false;
    for (char c : s) {
        if (base == 16 && !std::isxdigit(static_cast<unsigned char>(c))) return false;
        if (base == 10 && !std::isdigit(static_cast<unsigned char>(c))) return false;
        if (base == 2 && c != '0' && c != '1') return false;
    }
    unsigned long long v = std::strtoull(s.c_str(), nullptr, base);
    out = neg ? -static_cast<std::int64_t>(v) : static_cast<std::int64_t>(v);
    return true;
}

const ds::HashMap<std::string, int>& registerMap() {
    static ds::HashMap<std::string, int> m = [] {
        ds::HashMap<std::string, int> h;
        for (int i = 0; i < REG_COUNT; ++i) h.put(regName(i), i);
        return h;
    }();
    return m;
}

bool isLegacyRegister(const std::string& s) {
    static const char* legacy[] = {"eax", "ebx", "ecx", "edx", "esi", "edi", "esp", "ebp",
                                   "ax",  "bx",  "dx",  "si",  "di",  "sp",  "bp",  "al",
                                   "ah",  "bl",  "bh",  "dl",  "dh",  "ch"};
    for (const char* l : legacy) if (s == l) return true;
    return false;
}

std::vector<std::string> splitOperands(const std::string& s) {
    std::vector<std::string> out;
    std::string cur;
    int depth = 0;
    for (char c : s) {
        if (c == '[') ++depth;
        if (c == ']') --depth;
        if (c == ',' && depth == 0) { out.push_back(trim(cur)); cur.clear(); continue; }
        cur += c;
    }
    if (!trim(cur).empty()) out.push_back(trim(cur));
    return out;
}

struct SymbolTable {
    ds::HashMap<std::string, std::uint64_t> addr;
    Program* prog;
    bool autoDeclare = true;
    std::uint64_t lookup(const std::string& name, int line) {
        std::uint64_t a;
        if (addr.get(name, a)) return a;
        if (!autoDeclare) throw SyntaxError("Unknown variable '" + name + "'", line);
        a = CPU::kDataBase + 8 * prog->data.size();
        prog->data.push_back({name, a, 0});
        addr.put(name, a);
        return a;
    }
};

Operand parseOperand(const std::string& raw, int line, SymbolTable& syms, bool wantLabel) {
    Operand op;
    std::string s = trim(raw);
    op.text = s;
    std::string ls = lower(s);
    for (const char* sz : {"qword ptr ", "qword ", "ptr "})
        if (ls.rfind(sz, 0) == 0) { s = trim(s.substr(std::string(sz).size())); ls = lower(s); }

    if (!s.empty() && s.front() == '[') {
        if (s.back() != ']') throw SyntaxError("Missing ']' in memory operand '" + raw + "'", line);
        op.kind = OperandKind::Memory;
        std::string inner = s.substr(1, s.size() - 2);
        // split into signed terms
        std::vector<std::pair<int, std::string>> terms;
        std::string cur;
        int sign = 1;
        for (char c : inner) {
            if (c == '+' || c == '-') {
                if (!trim(cur).empty()) terms.push_back({sign, trim(cur)});
                cur.clear();
                sign = c == '-' ? -1 : 1;
                continue;
            }
            cur += c;
        }
        if (!trim(cur).empty()) terms.push_back({sign, trim(cur)});
        if (terms.empty()) throw SyntaxError("Empty memory operand '[]'", line);
        for (auto& [sg, t] : terms) {
            std::int64_t n;
            int r = Assembler::registerIndex(lower(t));
            if (r >= 0) {
                if (op.reg >= 0 || sg < 0) throw SyntaxError("Only one base register is supported in '" + raw + "'", line);
                op.reg = r;
            } else if (parseNumber(t, n)) {
                op.imm += sg * n;
            } else if (isIdent(t)) {
                if (!op.symbol.empty() || sg < 0) throw SyntaxError("Bad memory operand '" + raw + "'", line);
                op.symbol = t;
                op.symbolAddr = syms.lookup(t, line);
            } else {
                throw SyntaxError("Cannot understand '" + t + "' inside '" + raw + "'", line);
            }
        }
        return op;
    }

    if (ls == "cl") { op.kind = OperandKind::Register; op.reg = RCX; op.isCL = true; return op; }
    int r = Assembler::registerIndex(ls);
    if (r >= 0) { op.kind = OperandKind::Register; op.reg = r; return op; }
    if (isLegacyRegister(ls))
        throw SyntaxError("'" + s + "' is a 32/16/8-bit register - this CPU uses the 64-bit names (rax, rbx ...)", line);

    std::int64_t n;
    if (parseNumber(s, n)) { op.kind = OperandKind::Immediate; op.imm = n; return op; }

    if (isIdent(s)) {
        if (wantLabel) { op.kind = OperandKind::Label; op.symbol = s; return op; }
        throw SyntaxError("'" + s + "' is a memory variable - write [" + s + "] to use its value", line);
    }
    throw SyntaxError("Cannot understand operand '" + raw + "'", line);
}

} // namespace

int Assembler::registerIndex(const std::string& name) {
    int r = -1;
    registerMap().get(name, r);
    return r;
}

std::unique_ptr<Program> Assembler::assemble(const std::string& source, ds::HashMethod labelMethod) {
    auto prog = std::make_unique<Program>();
    prog->labelTable.reset(ds::makeHashTable(labelMethod, 7));

    std::istringstream in(source);
    std::string l;
    while (std::getline(in, l)) {
        if (!l.empty() && l.back() == '\r') l.pop_back();
        prog->lines.push_back(l);
    }

    SymbolTable syms;
    syms.prog = prog.get();

    struct Pending { int line; std::string mnem; std::string ops; std::string text; };
    std::vector<Pending> pending;
    bool inData = false;

    // ------------------------------------------------------------- pass 1
    for (int ln = 0; ln < int(prog->lines.size()); ++ln) {
        std::string s = prog->lines[ln];
        std::size_t sc = s.find(';');
        if (sc != std::string::npos) s = s.substr(0, sc);
        s = trim(s);
        if (s.empty()) continue;
        std::string ls = lower(s);
        if (ls.rfind("section", 0) == 0 || ls.rfind("segment", 0) == 0) {
            inData = ls.find(".data") != std::string::npos || ls.find(".bss") != std::string::npos;
            continue;
        }
        if (ls.rfind("global", 0) == 0 || ls.rfind("extern", 0) == 0 || ls.rfind("default", 0) == 0) continue;

        // data definition:  name dq value   /  name: dq value
        {
            std::istringstream ws(s);
            std::string a, b, rest;
            ws >> a >> b;
            std::getline(ws, rest);
            if (!a.empty() && a.back() == ':') a.pop_back();
            std::string lb = lower(b);
            if (lb == "dq" || lb == "dd" || lb == "dw" || lb == "db" || lb == "resq") {
                if (!isIdent(a)) throw SyntaxError("Bad variable name '" + a + "'", ln);
                std::int64_t v = 0;
                std::string rv = trim(rest);
                if (lb == "resq") rv = "0";
                if (!rv.empty() && !parseNumber(rv, v)) throw SyntaxError("Bad initial value '" + rv + "'", ln);
                if (syms.addr.contains(a)) throw SyntaxError("Variable '" + a + "' defined twice", ln);
                std::uint64_t addr = CPU::kDataBase + 8 * prog->data.size();
                prog->data.push_back({a, addr, v});
                syms.addr.put(a, addr);
                continue;
            }
        }
        if (inData) throw SyntaxError("Expected 'name dq value' in section .data", ln);

        // labels (possibly followed by an instruction)
        while (true) {
            std::size_t colon = s.find(':');
            if (colon == std::string::npos) break;
            std::string label = trim(s.substr(0, colon));
            if (!isIdent(label)) break;
            int existing;
            if (prog->labelTable->find(label, existing)) throw SyntaxError("Label '" + label + "' defined twice", ln);
            ds::ProbeTrace tr;
            prog->labelTable->insert(label, int(pending.size()), &tr);
            prog->labelTraces.push_back(tr);
            prog->labels.push_back({label, int(pending.size())});
            s = trim(s.substr(colon + 1));
        }
        if (s.empty()) continue;

        std::size_t sp = s.find_first_of(" \t");
        std::string mnem = lower(sp == std::string::npos ? s : s.substr(0, sp));
        std::string ops = sp == std::string::npos ? "" : trim(s.substr(sp));
        pending.push_back({ln, mnem, ops, s});
    }

    // ------------------------------------------------------------- pass 2
    const auto& table = OpcodeTable::instance();
    const auto& factory = InstructionFactory::instance();
    for (const auto& p : pending) {
        OpcodeLookup look;
        look.mnemonic = p.mnem;
        const OpcodeInfo* info = table.lookup(p.mnem, &look.probes);
        prog->opcodeLookups.push_back(look);
        if (!info) throw InvalidOpcodeError("Unknown instruction '" + p.mnem + "'", p.line);

        std::vector<std::string> raw = splitOperands(p.ops);
        if (int(raw.size()) < info->minOps || int(raw.size()) > info->maxOps) {
            std::ostringstream m;
            m << "'" << p.mnem << "' takes " << info->minOps << " operand" << (info->minOps == 1 ? "" : "s")
              << ", found " << raw.size();
            throw SyntaxError(m.str(), p.line);
        }
        bool wantLabel = info->klass == InstrClass::ControlFlow;
        std::vector<Operand> ops;
        for (const auto& r : raw) ops.push_back(parseOperand(r, p.line, syms, wantLabel));

        // operand rules
        if (wantLabel && !ops.empty()) {
            if (ops[0].kind != OperandKind::Label)
                throw SyntaxError("'" + p.mnem + "' needs a label as its target", p.line);
            int target;
            if (!prog->labelTable->find(ops[0].symbol, target))
                throw SyntaxError("Unknown label '" + ops[0].symbol + "'", p.line);
            ops[0].target = target;
        }
        if (!ops.empty() && !wantLabel && p.mnem != "push" && ops[0].kind == OperandKind::Immediate)
            throw SyntaxError("The destination of '" + p.mnem + "' cannot be a number", p.line);
        if (ops.size() == 2 && ops[0].kind == OperandKind::Memory && ops[1].kind == OperandKind::Memory)
            throw SyntaxError("x86 cannot use two memory operands in one instruction", p.line);
        if (p.mnem == "idiv" && ops[0].kind == OperandKind::Immediate)
            throw SyntaxError("idiv cannot divide by an immediate - load it into a register first", p.line);
        if ((p.mnem == "shl" || p.mnem == "shr" || p.mnem == "sal") && ops[1].kind != OperandKind::Immediate &&
            !ops[1].isCL)
            throw SyntaxError("Shift count must be a number or the cl register", p.line);
        if (ops.size() == 2 && ops[1].isCL && p.mnem != "shl" && p.mnem != "shr" && p.mnem != "sal")
            throw SyntaxError("'cl' is only supported as a shift count", p.line);

        prog->code.push_back(factory.create(*info, std::move(ops), p.line, p.text));
        prog->instrLine.push_back(p.line);
    }
    if (prog->code.empty()) throw SyntaxError("The program has no instructions", 0);
    return prog;
}

} // namespace vas::core
