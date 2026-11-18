// Turns English sentences such as 'add 12 and 3' into statements and marks the role of every word.

#include <string>
#include <vector>

// What each word of a sentence is, so the screen can colour it.
enum WordRole { FILLER, VERB, NUMBER, VARIABLE, OPERATOR_WORD, TARGET };

struct Word {
    std::string text;
    WordRole role;
};

// The result for one sentence: the generated statement and how each word was understood.
struct Translation {
    bool ok;
    std::string statement;           // for example "result = 12 + 3"
    std::string error;
    std::vector<Word> words;
};

// Turns plain English sentences into program statements with fixed sentence patterns.
class EnglishTranslator {
public:
    Translation translate(const std::string& sentence) {
        Translation result;
        result.ok = false;
        result.words = splitIntoWords(sentence);
        pos_ = 0;
        words_ = &result.words;

        if (accept("add")) {
            return finishAdd(result);
        }
        if (accept("set")) {
            return finishSet(result);
        }
        if (accept("increase")) {
            return finishIncrease(result);
        }
        result.error = "I do not know how to \"" + firstWord(result) + "\"";
        return result;
    }

private:
    // "add 12 and 3"  ->  result = 12 + 3
    Translation finishAdd(Translation& result) {
        std::string left = operand();
        if (!accept("and")) {
            return fail(result, "expected 'and' after the first number");
        }
        std::string right = operand();
        result.statement = "result = " + left + " + " + right;
        result.ok = true;
        return result;
    }

    // "set total to 5"  ->  total = 5
    Translation finishSet(Translation& result) {
        std::string target = name();
        if (!accept("to")) {
            return fail(result, "expected 'to' after the variable name");
        }
        std::string value = operand();
        (*words_)[1].role = TARGET;
        result.statement = target + " = " + value;
        result.ok = true;
        return result;
    }

    // "increase count by 2"  ->  count = count + 2
    Translation finishIncrease(Translation& result) {
        std::string target = name();
        std::string amount = "1";
        if (accept("by")) {
            amount = operand();
        }
        result.statement = target + " = " + target + " + " + amount;
        result.ok = true;
        return result;
    }

    // ---- helpers
    // looks at the next word; if it equals 'text' it is used up and marked as a verb or filler
    bool accept(const std::string& text) {
        if (pos_ >= words_->size()) {
            return false;
        }
        if ((*words_)[pos_].text != text) {
            return false;
        }
        (*words_)[pos_].role = VERB;
        pos_ = pos_ + 1;
        return true;
    }

    // a number or a variable name
    std::string operand() {
        if (pos_ >= words_->size()) {
            return "0";
        }
        Word& word = (*words_)[pos_];
        pos_ = pos_ + 1;
        bool isNumber = !word.text.empty() && word.text[0] >= '0' && word.text[0] <= '9';
        if (isNumber) {
            word.role = NUMBER;
        } else {
            word.role = VARIABLE;
        }
        return word.text;
    }

    std::string name() {
        if (pos_ >= words_->size()) {
            return "result";
        }
        (*words_)[pos_].role = VARIABLE;
        pos_ = pos_ + 1;
        return (*words_)[pos_ - 1].text;
    }

    Translation fail(Translation& result, const std::string& message) {
        result.ok = false;
        result.error = message;
        return result;
    }

    std::string firstWord(const Translation& result) {
        if (result.words.empty()) {
            return "";
        }
        return result.words[0].text;
    }

    // lower-case words separated by spaces
    std::vector<Word> splitIntoWords(const std::string& sentence) {
        std::vector<Word> words;
        std::string current;
        for (std::size_t i = 0; i <= sentence.size(); ++i) {
            char c = ' ';
            if (i < sentence.size()) {
                c = sentence[i];
            }
            if (c == ' ') {
                if (!current.empty()) {
                    Word word;
                    word.text = current;
                    word.role = FILLER;
                    words.push_back(word);
                    current = "";
                }
            } else {
                if (c >= 'A' && c <= 'Z') {
                    c = static_cast<char>(c - 'A' + 'a');
                }
                current = current + c;
            }
        }
        return words;
    }

    std::vector<Word>* words_;
    std::size_t pos_;
};
