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

class Translation {
    typedef std::string State;

  private:
    const TuringMachine &input_;
    transitions_t res_transitions_;

    SymbolSet alphabet_, states_;
    ImportantIdents important_idents_;

    // std::unordered_map<std::string, std::string> head_over_alphabet_;
    std::unordered_map<std::string, std::string> first_head_over_letter_;
    std::unordered_map<std::string, std::string> second_head_over_letter_;

    // (old state) -> (new state, scanning for the first letter)
    std::unordered_map<std::string, std::string> state_aliases_;
    std::unordered_map<State, State> state_cleanups_;
    std::unordered_map<State, State> state_moving_second_word_;

    void create_new_symbols_() {
        important_idents_.semi_start = states_.generate("semi-start");

        important_idents_.letter_tape_start = alphabet_.generate("tape_start");

        important_idents_.letter_word_end = alphabet_.generate("word_end");

        for (const std::string &letter : input_.working_alphabet()) {
            first_head_over_letter_[letter] = alphabet_.generate(letter + "_h");

            second_head_over_letter_[letter] =
                alphabet_.generate(letter + "__H");
        }

        for (const auto &state : input_.set_of_states()) {
            if (state == INITIAL_STATE) {
                state_aliases_[state] = important_idents_.semi_start;
                state_cleanups_[state] = states_.generate(state + "-cln");
            } else if (state == ACCEPTING_STATE || state == REJECTING_STATE) {
                state_aliases_[state] = state;
            } else {
                state_aliases_[state] = states_.generate(state);
                state_cleanups_[state] = states_.generate(state + "-cln");
            }
        }
    }

    inline void new_transition_(const State &initial,
                                const std::string &old_letter,
                                const State &final,
                                const std::string &new_letter,
                                const char &head_move) {
        const auto key = std::make_pair(initial, std::vector{old_letter});

        assert(res_transitions_.find(key) == res_transitions_.end());
        res_transitions_[key] = std::make_tuple(final, std::vector{new_letter},
                                                std::string{head_move});
    }

    void program_setup_() {
        // TODO
    }

    void program_steps_cleanup_() {
        for (const State &state : input_.set_of_states()) {
            const State state_cleanup_alias = state_cleanups_[state];

            for (const auto &letter : input_.working_alphabet()) {
                new_transition_(state_cleanup_alias, letter,
                                state_cleanup_alias, letter, HEAD_LEFT);
            }

            new_transition_(state_cleanup_alias,
                            important_idents_.letter_tape_start,
                            state_aliases_[state],
                            important_idents_.letter_tape_start, HEAD_RIGHT);
        }
    }

    void program_moving_second_word_(const State &target_state) {
        const State moving_state = states_.generate(target_state + "-mv");
        
    };

    void program_transition_(const State &new_identifier,
                             const State &old_initial,
                             const std::string &first_letter,
                             const std::string &second_letter) {

        const auto target = input_.transitions.find(std::make_pair(
            old_initial, std::vector{first_letter, second_letter}));

        if (target == input_.transitions.end()) {
            return;
        }

        // program letter change and head move
        const State second_head_erased = states_.generate(
            old_initial + "-" + first_letter + "-" + second_letter + "-she");
        new_transition_(new_identifier, second_head_over_letter_[second_letter],
                        second_head_erased, second_letter,
                        std::get<2>(target->second)[1]);

        const State go_to_first_head = states_.generate(
            old_initial + "-" + first_letter + "-" + second_letter + "-gtf");
        for (const auto &letter : input_.working_alphabet()) {
            // TODO blank???
            new_transition_(second_head_erased, letter, go_to_first_head,
                            second_head_over_letter_[letter], HEAD_LEFT);
        }

        new_transition_(go_to_first_head, important_idents_.letter_word_end,
                        go_to_first_head, important_idents_.letter_word_end,
                        HEAD_LEFT);

        for (const auto &letter : input_.working_alphabet()) {
            new_transition_(go_to_first_head, letter, go_to_first_head, letter,
                            HEAD_LEFT);
        }
        const State target_state = std::get<0>(target->second);
        const State first_head_erased = states_.generate(
            old_initial + "-" + first_letter + "-" + second_letter + "-fhe");

        new_transition_(go_to_first_head, first_head_over_letter_[first_letter],
                        first_head_erased, first_letter,
                        std::get<2>(target->second)[0]);

        for (const auto &letter : input_.working_alphabet()) {
            new_transition_(first_head_erased, letter,
                            state_cleanups_[target_state],
                            first_head_over_letter_[letter], HEAD_LEFT);
        }

        if (state_moving_second_word_.find(target_state) ==
            state_moving_second_word_.end()) {
            program_moving_second_word_(target_state);
        }

        new_transition_(first_head_erased, important_idents_.letter_word_end,
                        state_moving_second_word_[target_state],
                        first_head_over_letter_[BLANK], HEAD_RIGHT);
    }

    void create_states_grid_() {
        for (const auto &state : input_.set_of_states()) {

            if (state == ACCEPTING_STATE || state == REJECTING_STATE) {
                continue;
            }

            const auto looking_for_first_letter_ = state_aliases_[state];

            for (const auto &[first_letter, first_letter_with_head] :
                 first_head_over_letter_) {

                // didn't find the underlined letter
                new_transition_(looking_for_first_letter_, first_letter,
                                looking_for_first_letter_, first_letter,
                                HEAD_RIGHT);

                State looking_for_second_letter =
                    states_.generate(state + "-" + first_letter);

                // found the underlined letter
                new_transition_(looking_for_first_letter_,
                                first_letter_with_head,
                                looking_for_second_letter,
                                first_letter_with_head, HEAD_RIGHT);

                // move over end of the word symbol
                new_transition_(looking_for_second_letter,
                                important_idents_.letter_word_end,
                                looking_for_second_letter,
                                important_idents_.letter_word_end, HEAD_RIGHT);

                for (const auto &[second_letter, second_letter_with_head] :
                     second_head_over_letter_) {

                    // didn't find the underlined letter
                    new_transition_(looking_for_second_letter, second_letter,
                                    looking_for_second_letter, second_letter,
                                    HEAD_RIGHT);

                    // found the underlined letter
                    program_transition_(looking_for_second_letter, state,
                                        first_letter, second_letter);
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
    Translation(const TuringMachine &input)
        : input_(input), res_transitions_(),
          alphabet_(input.working_alphabet()), states_() {
        create_new_symbols_();

        program_setup_();

        program_steps_cleanup_();

        create_states_grid_();
    }

    TuringMachine result() {
        return TuringMachine(1, input_.input_alphabet, res_transitions_);
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
