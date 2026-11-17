// =====================================================================
//  English -> code translator                      [PSOOP CO3, CO4]
//  Plain English sentences are turned into statements the compiler
//  understands, e.g.
//      add 12 and 3                     ->  result = 12 + 3
//      set x to 5 plus y times 2        ->  x = 5 + y * 2
//      if x is greater than y then double x otherwise set y to 0
//      find the factorial of 5          ->  a while loop
//      repeat 3 times: increase count by 2
//  A small recursive-descent parser over words; every word gets a role
//  (verb, number, variable, operator, target) so the UI can colour how
//  the sentence was understood.
// =====================================================================
#pragma once
#include <string>
#include <vector>

namespace vas::core {

enum class WordRole { Filler, Verb, Number, Variable, Operator, Target, Keyword };

struct EnglishWord {
    std::string text;
    WordRole role = WordRole::Filler;
};

struct EnglishSentence {
    std::string text;
    int line = 0;
    std::string rule;                    // which pattern matched, e.g. "ADD"
    std::vector<EnglishWord> words;
    std::vector<std::string> code;       // generated statement lines
};

struct EnglishResult {
    bool ok = true;
    std::string error;
    int errorLine = -1;
    std::vector<EnglishSentence> sentences;
    std::string code;                    // all generated lines joined
};

class EnglishTranslator {
public:
    static EnglishResult translate(const std::string& source);
};

} // namespace vas::core
