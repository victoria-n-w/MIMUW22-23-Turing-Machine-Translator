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

struct TransitionTarget {
    std::string target_state, top_letter, bottom_letter;
    char top_head_move, bottom_head_move;
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
    class TopHead {
      public:
        static inline Translation::Letter
        this_other_not(const LetterEncoding &letter) {
            return letter.top_head;
        }

        static inline std::pair<Translation::Letter, Translation::Letter>
        make_pair(const Letter &this_letter, const Letter &other) {
            return std::make_pair(this_letter, other);
        }
    };

    class BottomHead {
      public:
        static inline Translation::Letter
        this_other_not(const LetterEncoding &letter) {
            return letter.bottom_head;
        }

        static inline std::pair<Translation::Letter, Translation::Letter>
        make_pair(const Letter &this_letter, const Letter &other) {
            return std::make_pair(other, this_letter);
        }
    };

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

    void program_transitions_();

    void program_reject_accept_(const SimulatedState &data,
                                const std::string &target);

    /**
     * Moves the top head, where the bottom one is
     * Leaves the actual head on the place where it was before performing hte
     * move
     */
    void program_move_top_(const TransitionTarget &traget_data,
                           const SimulatedState &data, const State &in_state,
                           const State &out_state);

    /**
     * Moves the bottom head, where the top one is
     * Leaves the actual head on the place where it was before performing hte
     * move
     */
    void program_move_bottom_(const TransitionTarget &traget_data,
                              const SimulatedState &data, const State &in_state,
                              const State &out_state);

    void program_cleanup_();

    void program_look_for_bottom_(const SimulatedState &data,
                                  const State &in_state,
                                  const State &out_state);

    void program_look_for_top_(const SimulatedState &data,
                               const State &in_state, const State &out_state);

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

        const auto target_data = TransitionTarget{
            .target_state = target_state,
            .top_letter = std::get<1>(target_it->second)[0],
            .bottom_letter = std::get<1>(target_it->second)[1],
            .top_head_move = std::get<2>(target_it->second)[0],
            .bottom_head_move = std::get<2>(target_it->second)[1]};

        const State move_top_first = states_.generate();
        const State move_bottom_first = states_.generate();

        for (const Letter &letter : input_.working_alphabet()) {
            new_transition_(
                input_state_alias,
                letters_map_[std::make_pair(data.top_letter, letter)].top_head,
                move_top_first,
                letters_map_[std::make_pair(data.top_letter, letter)].top_head,
                HEAD_STAY);
            new_transition_(
                input_state_alias,
                letters_map_[std::make_pair(letter, data.bottom_letter)]
                    .bottom_head,
                move_bottom_first,
                letters_map_[std::make_pair(letter, data.bottom_letter)]
                    .bottom_head,
                HEAD_STAY);
        }

        new_transition_(
            input_state_alias,
            letters_map_[std::make_pair(data.top_letter, data.bottom_letter)]
                .both_heads,
            move_top_first,
            letters_map_[std::make_pair(data.top_letter, data.bottom_letter)]
                .both_heads,
            HEAD_STAY);

        State intermiediate = states_.generate();
        const State found_bottom = states_.generate();
        program_move_top_(target_data, data, move_top_first, intermiediate);
        program_look_for_bottom_(data, intermiediate, found_bottom);
        program_move_bottom_(
            target_data, data, found_bottom,
            state_aliases_[target_data.target_state].going_back);

        const State found_top = states_.generate();
        intermiediate = states_.generate();
        program_move_bottom_(target_data, data, move_bottom_first,
                             intermiediate);
        program_look_for_top_(data, intermiediate, found_top);
        program_move_top_(target_data, data, found_top,
                          state_aliases_[target_data.target_state].going_back);
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

void Translation::program_move_top_(const TransitionTarget &target_data,
                                    const SimulatedState &data,
                                    const State &in_state,
                                    const State &out_state) {
    State marking_top_head = states_.generate();

    for (const Letter &bottom_letter : input_.working_alphabet()) {

        const auto encode_current_letter =
            letters_map_[std::make_pair(data.top_letter, bottom_letter)];

        const auto encode_target_letter =
            letters_map_[std::make_pair(target_data.top_letter, bottom_letter)];

        if (target_data.top_head_move == HEAD_STAY) {
            new_transition_(in_state, encode_current_letter.top_head, out_state,
                            encode_target_letter.top_head, HEAD_STAY);

            new_transition_(in_state, encode_current_letter.both_heads,
                            out_state, encode_target_letter.both_heads,
                            HEAD_STAY);
        } else {
            // just top head
            new_transition_(in_state, encode_current_letter.top_head,
                            marking_top_head, encode_current_letter.no_head,
                            target_data.top_head_move);
            // bottom head is in the same cell
            new_transition_(in_state, encode_current_letter.both_heads,
                            marking_top_head, encode_current_letter.bottom_head,
                            target_data.top_head_move);

            const auto return_move = target_data.top_head_move == HEAD_RIGHT
                                         ? HEAD_LEFT
                                         : HEAD_RIGHT;

            if (target_data.top_head_move == HEAD_RIGHT) {
                new_transition_(
                    marking_top_head, BLANK, out_state,
                    letters_map_[std::make_pair(BLANK, BLANK)].top_head,
                    return_move);
            }

            new_transition_(marking_top_head, encode_target_letter.no_head,
                            out_state, encode_target_letter.top_head,
                            return_move);
            new_transition_(marking_top_head, encode_target_letter.bottom_head,
                            out_state, encode_target_letter.both_heads,
                            return_move);
        }
    }
}

void Translation::program_move_bottom_(const TransitionTarget &traget_data,
                                       const SimulatedState &data,
                                       const State &in_state,
                                       const State &out_state) {

    const State marking_bottom_head = states_.generate();
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
