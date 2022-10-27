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
    TuringMachine result_;

    SymbolSet symbol_set_;
    ImportantStates important_states_;

    std::unordered_map<std::string, std::string> head_over_alphabet_;

    // (old state) -> (new state, scanning for the first letter)
    std::unordered_map<std::string, std::string> start_scanning_;

    // (old_state, 1st letter, 2nd letter) -> (new state identifier)
    std::unordered_map<std::tuple<std::string, std::string, std::string>,
                       std::string, hash_tuple>
        both_letters_found;

    void create_basic_states_(){
        // TODO pseudostart
    };

    void create_new_symbols_() {
        for (const std::string &letter : input_.working_alphabet()) {
            head_over_alphabet_[letter] = symbol_set_.generate(letter + "_H");
        }
    }

    void create_states_grid_() {

        for (const auto &state : input_.set_of_states()) {
            if (state == INITIAL_STATE) {
                start_scanning_[state] = important_states_.semi_start;
            } else {
                start_scanning_[state] = symbol_set_.generate(state);
            }
        }

        for (const auto &state : input_.set_of_states()) {
            const auto looking_for_first_letter = start_scanning_[state];

            for (const auto &[without_head, with_head] : head_over_alphabet_) {

                // didn't find the underlined letter
                result_.transitions[std::make_pair(looking_for_first_letter,
                                                   std::vector{without_head})] =
                    std::make_tuple(looking_for_first_letter,
                                    std::vector{without_head}, HEAD_RIGHT);

                State looking_for_second_letter =
                    symbol_set_.generate(state + "-" + without_head);

                // found the underlined letter
                result_.transitions[std::make_pair(looking_for_first_letter,
                                                   std::vector{with_head})] =
                    std::make_tuple(looking_for_second_letter,
                                    std::vector{with_head}, HEAD_RIGHT);

                for (const auto &[second_letter, second_letter_with_head] :
                     head_over_alphabet_) {
                    // TODO czy sie mogą powatarzac symbole w alfabecie i
                    // statnach

                    result_.transitions[std::make_pair(
                        looking_for_second_letter,
                        std::vector{second_letter})] =
                        std::make_tuple(looking_for_second_letter,
                                        std::vector{second_letter}, HEAD_RIGHT);

                    const std::string going_back = symbol_set_.generate(
                        state + "-" + without_head + "-" + second_letter);

                    both_letters_found[std::make_tuple(
                        state, without_head, second_letter)] = going_back;

                    result_.transitions[std::make_pair(
                        looking_for_second_letter,
                        std::vector{second_letter_with_head})] =
                        std::make_tuple(going_back,
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
        : input_(input), result_(TuringMachine{0, {}, {}}), symbol_set_(input) {
        create_basic_states_();

        create_new_symbols_();

        create_states_grid_();
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
}