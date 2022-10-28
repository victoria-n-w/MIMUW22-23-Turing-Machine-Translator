#include <cassert>
#include <iostream>
#include <string>
#include <unordered_map>

#include "symbol_set.h"
#include "turing_machine.h"

struct ImportantIdents {
    std::string semi_start;
    std::string letter_tape_start;
    std::string letter_word_end;
};

struct LetterEncoding {
    std::string no_head, first_head, second_head, both_heads;
};

struct pairhash {
    size_t operator()(const std::pair<std::string, std::string> &p) const {
        return std::hash<std::string>()(p.first + p.second);
    }
};

class Translation {
    typedef std::string State;
    typedef std::string Letter;

  public:
    Translation(const TuringMachine &intput);

    TuringMachine result();

  private:
    void create_double_letters_();

    std::unordered_map<std::pair<std::string, std::string>, LetterEncoding,
                       pairhash>
        letters_map_;
    const TuringMachine &input_;
    SymbolSet states_, letters_;
};

Translation::Translation(const TuringMachine &input)
    : input_(input), states_(), letters_(input.working_alphabet()) {
    create_double_letters_();
}

int main(int argc, char *argv[]) {
    std::string filename = argv[1];
    // TODO incorrect number of arguments

    FILE *f = fopen(filename.c_str(), "r");
    if (!f) {
        std::cerr << "ERROR: File " << filename << " does not exist\n";
        return 1;
    }

    auto tm = read_tm_from_file(f);

    Translation translation(tm);

    std::cout << translation.result();
}
