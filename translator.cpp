#include <iostream>
#include <string>
#include <unordered_map>

#include "turing_machine.h"

class Translation {
  private:
    const TuringMachine &input_;
    TuringMachine result_;

    void create_basic_states_(){};

    void create_states_grid_(){};

    void create_new_symbols_() {
        for (const std::string &letter : input_.working_alphabet()) {
        }
    }

  public:
    Translation(const TuringMachine &input)
        : input_(input), result_(TuringMachine{0, {}, {}}) {
        create_basic_states_();

        create_new_symbols_();

        for (const auto &state : input_.set_of_states()) {
            for (const auto &letter_a : input_.working_alphabet()) {
                for (const auto &letter_b : input_.working_alphabet()) {
                }
            }
        }
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