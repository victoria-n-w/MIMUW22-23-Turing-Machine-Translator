#include <iostream>
#include <string>
#include <unordered_map>

#include "symbol_set.h"
#include "turing_machine.h"

struct ImportantStates {
    std::string semi_start;
};

struct hash_tuple {
    size_t operator()(
        const std::tuple<std::string, std::string, std::string> &x) const {
        std::hash<std::string> h{};
        return h(std::get<0>(x)) ^ h(std::get<1>(x)) ^ h(std::get<2>(x));
    }
};

class Translation {
  private:
    const TuringMachine &input_;
    transitions_t result_transitions_;

    SymbolSet res_alphabet_, res_states_;
    ImportantStates important_states_;

    std::unordered_map<std::string, std::string> head_over_alphabet_;

    // (old state) -> (new state, scanning for the first letter)
    std::unordered_map<std::string, std::string> start_scanning_;

    void create_basic_states_(){
        // TODO pseudostart
    };

    void create_new_symbols_() {
        for (const std::string &letter : input_.working_alphabet()) {
            head_over_alphabet_[letter] = res_alphabet_.generate(letter + "_H");
        }
    }

    void create_states_grid_() {

        for (const auto &state : input_.set_of_states()) {
            if (state == INITIAL_STATE) {
                start_scanning_[state] = important_states_.semi_start;
            } else {
                start_scanning_[state] = res_states_.generate(state);
            }
        }

        for (const auto &state : input_.set_of_states()) {
            const auto looking_for_first_letter = start_scanning_[state];

            for (const auto &[first_letter, first_letter_with_head] :
                 head_over_alphabet_) {

                // didn't find the underlined letter
                result_transitions_[std::make_pair(looking_for_first_letter,
                                                   std::vector{first_letter})] =
                    std::make_tuple(looking_for_first_letter,
                                    std::vector{first_letter}, HEAD_RIGHT);

                State looking_for_second_letter =
                    res_states_.generate(state + "-" + first_letter);

                // found the underlined letter
                result_transitions_[std::make_pair(
                    looking_for_first_letter,
                    std::vector{first_letter_with_head})] =
                    std::make_tuple(looking_for_second_letter,
                                    std::vector{first_letter_with_head},
                                    HEAD_RIGHT);

                for (const auto &[second_letter, second_letter_with_head] :
                     head_over_alphabet_) {
                    // TODO czy sie mogą powatarzac symbole w alfabecie i
                    // statnach

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
        create_basic_states_();

        create_new_symbols_();

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
