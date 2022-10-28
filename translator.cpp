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

struct StateEncoding {
    std::string start_scanning, going_back;
};

struct SimulatedState {
    std::string state, top_letter, bottom_letter;
};

struct pairhash {
    size_t operator()(const std::pair<std::string, std::string> &p) const {
        return std::hash<std::string>()(p.first + p.second);
    }
};

class Translation {
    typedef std::string State;
    typedef std::string Letter;

    typedef std::unordered_map<std::pair<std::string, std::string>,
                               LetterEncoding, pairhash>
        LettersMap;

  public:
    Translation(const TuringMachine &intput);

    TuringMachine result();

  private:
    LettersMap create_double_letters_();

    ImportantIdents create_important_idents_();

    std::unordered_map<State, StateEncoding> create_state_aliases_();

    std::vector<std::pair<SimulatedState, State>>
    create_simulated_states_aliases_();

    void program_setup_protocol_(); // TODO

    void program_scanning_for_letters_(); // TODO

    void program_transitions_(); // TODO

    void program_expanding_the_word_(); // TODO

    const TuringMachine &input_;
    SymbolSet states_, letters_;
    LettersMap letters_map_;
    ImportantIdents importandt_idents_;
    std::unordered_map<State, StateEncoding> state_aliases_;
    std::vector<std::pair<SimulatedState, State>> simulated_states_aliases_;

    transitions_t res_transition_;
};

Translation::Translation(const TuringMachine &input)
    : input_(input),
      // we cannot allow for those identifiers to be generated
      states_(std::vector<std::string>{INITIAL_STATE, ACCEPTING_STATE,
                                       REJECTING_STATE}),
      letters_(input.working_alphabet()),
      letters_map_(create_double_letters_()),
      importandt_idents_(create_important_idents_()),
      state_aliases_(create_state_aliases_()),
      simulated_states_aliases_(create_simulated_states_aliases_()) {
    program_setup_protocol_();

    program_scanning_for_letters_();

    program_transitions_();

    program_expanding_the_word_();
}

Translation::LettersMap Translation::create_double_letters_() {
    LettersMap res;
    for (const Letter &letter_top : input_.working_alphabet()) {
        for (const Letter &letter_bottom : input_.working_alphabet()) {
            res[std::make_pair(letter_top, letter_bottom)] = LetterEncoding{
                .no_head = letters_.generate(letter_top + "-" + letter_bottom),
                .first_head =
                    letters_.generate(letter_top + "_H-" + letter_bottom),
                .second_head =
                    letters_.generate(letter_top + "-" + letter_bottom + "_H"),
                .both_heads = letters_.generate(letter_top + "_H-" +
                                                letter_bottom + "_H")};
        }
    }
    return res;
}

ImportantIdents Translation::create_important_idents_() {
    return ImportantIdents{
        .semi_start = letters_.generate("semi-start"),
        .letter_tape_start = letters_.generate("tape-start"),
        .letter_word_end = letters_.generate("word-end"),
    };
}

std::unordered_map<Translation::State, StateEncoding>
Translation::create_state_aliases_() {
    std::unordered_map<std::string, StateEncoding> res;
    for (const State &state : input_.set_of_states()) {
        res[state] = StateEncoding{
            .start_scanning = states_.generate(state + "-start-scanning"),
            .going_back = states_.generate(state + "-going-back"),
        };
    }
    return res;
}

std::vector<std::pair<SimulatedState, Translation::State>>
Translation::create_simulated_states_aliases_() {
    std::vector<std::pair<SimulatedState, Translation::State>> res;
    for (const State &state : input_.set_of_states()) {
        for (const Letter &top_letter : input_.working_alphabet()) {
            for (const Letter &bottom_letter : input_.working_alphabet()) {
                res.push_back(std::make_pair(
                    SimulatedState{
                        .state = state,
                        .top_letter = top_letter,
                        .bottom_letter = bottom_letter,
                    },
                    states_.generate(state + "-" + top_letter + "-" +
                                     bottom_letter)));
            }
        }
    }
}

void Translation::program_setup_protocol_() {}

void Translation::program_scanning_for_letters_() {}

void Translation::program_transitions_() {}

void Translation::program_expanding_the_word_() {}

TuringMachine Translation::result() {
    return TuringMachine(1, input_.input_alphabet, res_transition_);
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
