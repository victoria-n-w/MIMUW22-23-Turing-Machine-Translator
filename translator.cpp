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

class Translation {
  private:
    const TuringMachine &input_;
    transitions_t result_transitions_;

    SymbolSet res_alphabet_, res_states_;
    ImportantIdents important_states_;

    std::unordered_map<std::string, std::string> head_over_alphabet_;

    // (old state) -> (new state, scanning for the first letter)
    std::unordered_map<std::string, std::string> state_aliases_;

    void create_new_symbols_() {
        important_states_.semi_start = res_states_.generate("semi-start");

        important_states_.letter_tape_start =
            res_alphabet_.generate("tape_start");

        important_states_.letter_word_end = res_alphabet_.generate("word_end");

        for (const std::string &letter : input_.working_alphabet()) {
            head_over_alphabet_[letter] = res_alphabet_.generate(letter + "_H");
        }
    }

    void program_setup_() {
        // TODO
    }

    void program_transition_(const std::string &new_identifier,
                             const std::string &old_initial,
                             const std::string &first_letter,
                             const std::string &second_letter) {
        for (const auto &[letter, letter_with_head] : head_over_alphabet_) {
            result_transitions_[std::make_pair(new_identifier,
                                               std::vector{letter})] =
                std::make_tuple(new_identifier, std::vector{letter}, HEAD_LEFT);

            result_transitions_[std::make_pair(new_identifier,
                                               std::vector{letter_with_head})] =
                std::make_tuple(new_identifier, std::vector{letter_with_head},
                                HEAD_LEFT);
        }
    }

    void create_states_grid_() {

        for (const auto &state : input_.set_of_states()) {
            if (state == INITIAL_STATE) {
                state_aliases_[state] = important_states_.semi_start;
            } else if (state == ACCEPTING_STATE || state == REJECTING_STATE) {
                state_aliases_[state] = state;
            } else {
                state_aliases_[state] = res_states_.generate(state);
            }
        }

        for (const auto &state : input_.set_of_states()) {

            if (state == ACCEPTING_STATE || state == REJECTING_STATE) {
                continue;
            }

            const auto looking_for_first_letter_ = state_aliases_[state];

            for (const auto &[first_letter, first_letter_with_head] :
                 head_over_alphabet_) {

                // didn't find the underlined letter
                result_transitions_[std::make_pair(looking_for_first_letter_,
                                                   std::vector{first_letter})] =
                    std::make_tuple(looking_for_first_letter_,
                                    std::vector{first_letter}, HEAD_RIGHT);

                State looking_for_second_letter =
                    res_states_.generate(state + "-" + first_letter);

                // found the underlined letter
                result_transitions_[std::make_pair(
                    looking_for_first_letter_,
                    std::vector{first_letter_with_head})] =
                    std::make_tuple(looking_for_second_letter,
                                    std::vector{first_letter_with_head},
                                    HEAD_RIGHT);

                for (const auto &[second_letter, second_letter_with_head] :
                     head_over_alphabet_) {
                    result_transitions_[std::make_pair(
                        looking_for_second_letter,
                        std::vector{second_letter})] =
                        std::make_tuple(looking_for_second_letter,
                                        std::vector{second_letter}, HEAD_RIGHT);

                    const State simulated_state = res_states_.generate(
                        state + "-" + first_letter + "-" + second_letter);

                    result_transitions_[std::make_pair(
                        looking_for_second_letter,
                        std::vector{second_letter_with_head})] =
                        std::make_tuple(simulated_state,
                                        std::vector{second_letter_with_head},
                                        HEAD_LEFT);

                    program_transition_(simulated_state, state, first_letter,
                                        second_letter);
                }
            }
        }
    }

    transitions_t make_transition(const std::string &state,
                                  const std::string &target_state,
                                  const std::string &letter_read,
                                  const std::string &letter_written,
                                  const std::string &move) {}

  public:
    typedef std::string State;

    Translation(const TuringMachine &input)
        : input_(input), result_transitions_(),
          res_alphabet_(input.working_alphabet()), res_states_() {
        create_new_symbols_();

        program_setup_();

        create_states_grid_();
    }

    TuringMachine result() {
        return TuringMachine(1, input_.input_alphabet, result_transitions_);
    }
};

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
