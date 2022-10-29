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
    std::string no_head, top_head, bottom_head, both_heads;
};

struct StateEncoding {
    std::string do_scanning, going_back;
};

struct SimulatedState {
    std::string state, top_letter, bottom_letter;
};

struct SimulatedTransition {
    std::string target_state, top_letter, bottom_letter;
    char top_head_move, bottom_head_move;

    bool operator==(const SimulatedTransition &other) const {
        return top_head_move == other.top_head_move &&
               bottom_head_move == other.bottom_head_move &&
               target_state == other.target_state &&
               top_letter == other.top_letter &&
               bottom_letter == other.bottom_letter;
    }
};

struct MovingState {
    std::string top_bottom, bottom_top, both;
};

struct hash_transition {
    size_t operator()(const SimulatedTransition &p) const {
        return std::hash<std::string>()(p.target_state + " " + p.top_letter +
                                        " " + p.bottom_letter +
                                        p.top_head_move + p.bottom_head_move);
    }
};

struct FoundLetterEncodig {
    std::string top_found, bottom_found;
};

struct pairhash {
    size_t operator()(const std::pair<std::string, std::string> &p) const {
        return std::hash<std::string>()(p.first + " " + p.second);
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

    // skips REJECTING and ACCEPTING states
    std::unordered_map<State, StateEncoding> create_state_aliases_();

    /**
     * Moves the input word one letter to the right, writing the tape begin
     * symbol
     * Leaves head on the first letter of the word, right after the tape begin
     * symbol
     */
    void program_setup_protocol_();

    /**
     * The head is on the first letter of the word,
     * right after the begginig of the tape symbol
     *
     * Leaves the head on the letter found second
     */
    void program_scanning_for_letters_();

    void program_transitions_(); // TODO

    void program_reject_accept_(const SimulatedState &data,
                                const std::string &target);

    MovingState program_transition_(const SimulatedTransition &sim_transition);

    void program_move_top_(const SimulatedTransition &sim_transition,
                           const State &in_state, const State &out_state);

    void program_move_bottom_(const SimulatedTransition &sim_transition,
                              const State &in_state, const State &out_state);

    void program_move_both_(const SimulatedTransition &sim_transition,
                            const State &input_state);

    void program_cleanup_();

    inline void new_transition_(const State &initial,
                                const std::string &old_letter,
                                const State &final,
                                const std::string &new_letter,
                                const char &head_move);

    const TuringMachine &input_;
    SymbolSet states_, letters_;
    LettersMap letters_map_;
    ImportantIdents importandt_idents_;
    std::unordered_map<State, StateEncoding> state_aliases_;
    std::vector<std::pair<SimulatedState, State>> simulated_states_aliases_;

    transitions_t res_transitions_;
};

Translation::Translation(const TuringMachine &input)
    : input_(input),
      // we cannot allow for those identifiers to be generated
      states_(std::vector<std::string>{INITIAL_STATE, ACCEPTING_STATE,
                                       REJECTING_STATE}),
      letters_(input.working_alphabet()),
      letters_map_(create_double_letters_()),
      importandt_idents_(create_important_idents_()),
      state_aliases_(create_state_aliases_()) {

    program_setup_protocol_();

    program_scanning_for_letters_();

    program_transitions_();

    program_cleanup_();
}

Translation::LettersMap Translation::create_double_letters_() {
    LettersMap res;
    for (const Letter &letter_top : input_.working_alphabet()) {
        for (const Letter &letter_bottom : input_.working_alphabet()) {
            res[std::make_pair(letter_top, letter_bottom)] = LetterEncoding{
                .no_head = letters_.generate(letter_top + "-" + letter_bottom),
                .top_head =
                    letters_.generate(letter_top + "_H-" + letter_bottom),
                .bottom_head =
                    letters_.generate(letter_top + "-" + letter_bottom + "_H"),
                .both_heads = letters_.generate(letter_top + "_H-" +
                                                letter_bottom + "_H")};
        }
    }
    return res;
}

ImportantIdents Translation::create_important_idents_() {
    return ImportantIdents{
        .semi_start = states_.generate("semi-start"),
        .letter_tape_start = letters_.generate("tape-start"),
        .letter_word_end = letters_.generate("word-end"),
    };
}

std::unordered_map<Translation::State, StateEncoding>
Translation::create_state_aliases_() {
    std::unordered_map<std::string, StateEncoding> res;
    for (const State &state : input_.set_of_states()) {
        if (state == INITIAL_STATE) {
            res[state] = StateEncoding{
                .do_scanning = importandt_idents_.semi_start,
                .going_back = states_.generate(state + "-going-back"),
            };
        } else if (state != REJECTING_STATE && state != ACCEPTING_STATE) {
            res[state] = StateEncoding{
                .do_scanning = states_.generate(state + "-start-scanning"),
                .going_back = states_.generate(state + "-going-back"),
            };
        }
        // we skip the accepting and rejecting state
        // because we will never start processing them (getting to them means
        // end of the execution)
    }
    return res;
}

void Translation::program_setup_protocol_() {

    std::vector<std::pair<Letter, State>> moing_first_letter;

    /**
     * special case for empty input (head is over blank)
     */
    const auto move_first_blank = states_.generate("move-first-blank");
    new_transition_(INITIAL_STATE, BLANK, move_first_blank,
                    importandt_idents_.letter_tape_start, HEAD_RIGHT);
    new_transition_(move_first_blank, BLANK, move_first_blank,
                    letters_map_[std::make_pair(BLANK, BLANK)].both_heads,
                    HEAD_LEFT);
    new_transition_(move_first_blank, importandt_idents_.letter_tape_start,
                    state_aliases_[INITIAL_STATE].do_scanning,
                    importandt_idents_.letter_tape_start, HEAD_RIGHT);

    for (const Letter &letter : input_.input_alphabet) {
        const State move_curr_letter =
            states_.generate("setup-move-1st-" + letter);
        moing_first_letter.push_back(std::make_pair(letter, move_curr_letter));

        new_transition_(INITIAL_STATE, letter, move_curr_letter,
                        importandt_idents_.letter_tape_start, HEAD_RIGHT);
    }

    std::unordered_map<Letter, State> moving_letter;
    // if we we were to move a blank, go to the begining
    // moving_letter[BLANK] = state_aliases_[INITIAL_STATE].going_back;

    // TODO remove: for debug purpose only

    for (const Letter &letter : input_.input_alphabet) {

        const auto moving_letter_state =
            states_.generate("setup-move-" + letter);
        moving_letter[letter] = moving_letter_state;

        for (const auto &[letter_to_write, state] : moing_first_letter) {
            new_transition_(
                state, letter, moving_letter_state,
                letters_map_[std::make_pair(letter_to_write, BLANK)].both_heads,
                HEAD_RIGHT);
        }
    }

    for (const Letter &letter_moved : input_.input_alphabet) {
        for (const Letter &curr_letter : input_.input_alphabet) {
            new_transition_(
                moving_letter[letter_moved], curr_letter,
                moving_letter[curr_letter],
                letters_map_[std::make_pair(letter_moved, BLANK)].no_head,
                HEAD_RIGHT);
        }
    }

    for (const Letter &letter_moved : input_.input_alphabet) {
        new_transition_(
            moving_letter[letter_moved], BLANK,
            state_aliases_[INITIAL_STATE].going_back,
            letters_map_[std::make_pair(letter_moved, BLANK)].no_head,
            HEAD_LEFT);
    }
}

void Translation::program_scanning_for_letters_() {
    std::unordered_map<std::pair<std::string, std::string>, FoundLetterEncodig,
                       pairhash>
        found_letter_encoding;

    for (const auto &[old_state_ident, state_alias] : state_aliases_) {

        for (const Letter &letter : input_.working_alphabet()) {
            found_letter_encoding[std::make_pair(old_state_ident, letter)] =
                FoundLetterEncodig{
                    .top_found =
                        states_.generate(old_state_ident + "-T-" + letter),
                    .bottom_found =
                        states_.generate(old_state_ident + "-B-" + letter),
                };
        }

        for (const auto &[letters_pair, letter_encoding] : letters_map_) {

            auto it = found_letter_encoding.find(
                std::make_pair(old_state_ident, letters_pair.first));

            assert(it != found_letter_encoding.end());

            const State top_letter_found = it->second.top_found;

            it = found_letter_encoding.find(
                std::make_pair(old_state_ident, letters_pair.second));

            assert(it != found_letter_encoding.end());

            const State bottom_letter_found = it->second.bottom_found;

            new_transition_(state_alias.do_scanning, letter_encoding.no_head,
                            state_alias.do_scanning, letter_encoding.no_head,
                            HEAD_RIGHT);

            new_transition_(state_alias.do_scanning, letter_encoding.top_head,
                            top_letter_found, letter_encoding.top_head,
                            HEAD_RIGHT);

            new_transition_(state_alias.do_scanning,
                            letter_encoding.bottom_head, bottom_letter_found,
                            letter_encoding.bottom_head, HEAD_RIGHT);

            const State found_both_letters =
                states_.generate(old_state_ident + "-" + letters_pair.first +
                                 "-" + letters_pair.second);

            simulated_states_aliases_.push_back(std::make_pair(
                SimulatedState{
                    .state = old_state_ident,
                    .top_letter = letters_pair.first,
                    .bottom_letter = letters_pair.second,
                },
                found_both_letters));

            new_transition_(state_alias.do_scanning, letter_encoding.both_heads,
                            found_both_letters, letter_encoding.both_heads,
                            HEAD_STAY);

            new_transition_(top_letter_found, letter_encoding.no_head,
                            top_letter_found, letter_encoding.no_head,
                            HEAD_RIGHT);

            new_transition_(top_letter_found, letter_encoding.bottom_head,
                            found_both_letters, letter_encoding.bottom_head,
                            HEAD_STAY);

            new_transition_(bottom_letter_found, letter_encoding.no_head,
                            bottom_letter_found, letter_encoding.no_head,
                            HEAD_RIGHT);

            new_transition_(bottom_letter_found, letter_encoding.top_head,
                            found_both_letters, letter_encoding.top_head,
                            HEAD_STAY);
        }
    }
}

void Translation::program_transitions_() {

    std::unordered_map<SimulatedTransition, MovingState, hash_transition>
        transition_states;

    for (const auto &it : simulated_states_aliases_) {
        const auto &data = it.first;
        const auto &input_state_alias = it.second;

        const auto target_it = input_.transitions.find(std::make_pair(
            data.state, std::vector{data.top_letter, data.bottom_letter}));

        if (target_it == input_.transitions.end()) {
            continue;
        }

        const auto &target_state = std::get<0>(target_it->second);
        if (target_state == ACCEPTING_STATE ||
            target_state == REJECTING_STATE) {
            program_reject_accept_(data, target_state);
            return;
        }

        const auto simulated_transition = SimulatedTransition{
            .target_state = target_state,
            .top_letter = std::get<1>(target_it->second)[0],
            .bottom_letter = std::get<1>(target_it->second)[1],
            .top_head_move = std::get<2>(target_it->second)[0],
            .bottom_head_move = std::get<2>(target_it->second)[1]};

        auto transition_state_ptr =
            transition_states.find(simulated_transition);

        MovingState transition_state;

        if (transition_state_ptr == transition_states.end()) {
            transition_state = program_transition_(simulated_transition);
            transition_states[simulated_transition] = transition_state;
        } else {
            transition_state = transition_state_ptr->second;
        }

        for (const Letter &letter : input_.working_alphabet()) {
            // on current symbol, head on top
            new_transition_(
                input_state_alias,
                letters_map_[std::make_pair(data.top_letter, letter)].top_head,
                transition_state.top_bottom,
                letters_map_[std::make_pair(data.top_letter, letter)].top_head,
                HEAD_STAY);

            // on current symbol head on bottom
            new_transition_(
                input_state_alias,
                letters_map_[std::make_pair(letter, data.bottom_letter)]
                    .bottom_head,
                transition_state.bottom_top,
                letters_map_[std::make_pair(letter, data.bottom_letter)]
                    .bottom_head,
                HEAD_STAY);
        }
        // both heads on current symbol

        new_transition_(
            input_state_alias,
            letters_map_[std::make_pair(data.top_letter, data.bottom_letter)]
                .both_heads,
            transition_state.both,
            letters_map_[std::make_pair(data.top_letter, data.bottom_letter)]
                .both_heads,
            HEAD_STAY);
    }
}

void Translation::program_reject_accept_(const SimulatedState &data,
                                         const std::string &target) {
    for (const auto &letter : input_.working_alphabet()) {

        // on current symbol, head is on top
        new_transition_(
            data.state,
            letters_map_[std::make_pair(data.top_letter, letter)].top_head,
            target, BLANK, HEAD_STAY);

        // on current symbol head is on bottom
        new_transition_(data.state,
                        letters_map_[std::make_pair(letter, data.bottom_letter)]
                            .bottom_head,
                        target, BLANK, HEAD_STAY);
    }

    new_transition_(
        data.state,
        letters_map_[std::make_pair(data.top_letter, data.bottom_letter)]
            .both_heads,
        target, BLANK, HEAD_STAY);
}

MovingState
Translation::program_transition_(const SimulatedTransition &sim_transition) {

    const State moving_both_heads = states_.generate(
        sim_transition.target_state + "-" + sim_transition.top_letter + "-" +
        sim_transition.bottom_letter + sim_transition.bottom_head_move +
        sim_transition.top_head_move);

    const State move_top_first = states_.generate();

    State intermiediate = states_.generate();
    program_move_top_(sim_transition, move_top_first, intermiediate);
    program_move_bottom_(
        sim_transition, intermiediate,
        state_aliases_[sim_transition.target_state].going_back);

    const State move_bottom_first = states_.generate();

    intermiediate = states_.generate();
    program_move_bottom_(sim_transition, move_bottom_first, intermiediate);
    program_move_top_(sim_transition, intermiediate,
                      state_aliases_[sim_transition.target_state].going_back);

    if (sim_transition.bottom_head_move == sim_transition.top_head_move) {
        program_move_both_(sim_transition, moving_both_heads);
    } else {
    }
    return MovingState{.top_bottom = move_top_first,
                       .bottom_top = move_bottom_first,
                       .both = moving_both_heads};
}

void Translation::program_move_top_(const SimulatedTransition &sim_transition,
                                    const State &in_state,
                                    const State &out_state) {}

void Translation::program_move_bottom_(
    const SimulatedTransition &sim_transition, const State &in_state,
    const State &out_state) {}

void Translation::program_move_both_(const SimulatedTransition &sim_transition,
                                     const State &input_state) {
    const State intermidiate = states_.generate(
        sim_transition.target_state + "-" + sim_transition.top_letter + "-" +
        sim_transition.bottom_letter + sim_transition.bottom_head_move +
        sim_transition.top_head_move);
    for (const auto &[letters_pair, encoding] : letters_map_) {
        if (sim_transition.top_head_move == HEAD_STAY) {
            new_transition_(
                input_state, encoding.both_heads,
                state_aliases_[sim_transition.target_state].going_back,
                letters_map_[std::make_pair(sim_transition.top_letter,
                                            sim_transition.bottom_letter)]
                    .both_heads,
                HEAD_LEFT);
        } else {
            new_transition_(input_state, encoding.both_heads, intermidiate,
                            encoding.no_head, sim_transition.top_head_move);

            new_transition_(
                intermidiate, encoding.no_head,
                state_aliases_[sim_transition.target_state].going_back,
                encoding.both_heads, HEAD_LEFT);
        }
    }
    if (sim_transition.top_head_move == HEAD_RIGHT) {
        // special case, we moved into a newly written blank
        // this can only happen if we move the heads to the right
        new_transition_(intermidiate, BLANK,
                        state_aliases_[sim_transition.target_state].going_back,
                        letters_map_[std::make_pair(BLANK, BLANK)].both_heads,
                        HEAD_LEFT);
    }
}

void Translation::program_cleanup_() {
    for (const auto &[old_state_ident, state_alias] : state_aliases_) {
        assert(state_alias.going_back != "");
        for (const auto &[key, letters_encoding] : letters_map_) {
            new_transition_(state_alias.going_back, letters_encoding.no_head,
                            state_alias.going_back, letters_encoding.no_head,
                            HEAD_LEFT);
        }

        // once we reach the begging of the tape, start scanning for letters
        new_transition_(state_alias.going_back,
                        importandt_idents_.letter_tape_start,
                        state_alias.do_scanning,
                        importandt_idents_.letter_tape_start, HEAD_RIGHT);
    }
}

void Translation::new_transition_(const State &initial,
                                  const std::string &old_letter,
                                  const State &final,
                                  const std::string &new_letter,
                                  const char &head_move) {
    const auto key = std::make_pair(initial, std::vector{old_letter});

    assert(res_transitions_.find(key) == res_transitions_.end());

    res_transitions_[key] =
        std::make_tuple(final, std::vector{new_letter}, std::string{head_move});
}

TuringMachine Translation::result() {
    return TuringMachine(1, input_.input_alphabet, res_transitions_);
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
