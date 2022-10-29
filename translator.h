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

        static inline Translation::Letter
        other_this_not(const LetterEncoding &letter) {
            return letter.bottom_head;
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

        static inline Translation::Letter
        other_this_not(const LetterEncoding &letter) {
            return letter.top_head;
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

    template <typename H>
    void program_move_(const Letter &target_letter,
                       const char &target_head_move, const Letter &this_letter,
                       const State &in_state, const State &out_state) {

        State marking_this_head = states_.generate();

        for (const Letter &other_letter : input_.working_alphabet()) {

            const auto encode_current_letter =
                letters_map_[H::make_pair(this_letter, other_letter)];

            const auto encode_target_letter =
                letters_map_[H::make_pair(target_letter, other_letter)];

            if (target_head_move == HEAD_STAY) {
                new_transition_(
                    in_state, H::this_other_not(encode_current_letter),
                    out_state, H::this_other_not(encode_target_letter),
                    HEAD_STAY);

                new_transition_(in_state, encode_current_letter.both_heads,
                                out_state, encode_target_letter.both_heads,
                                HEAD_STAY);
            } else {
                // just top head
                new_transition_(
                    in_state, H::this_other_not(encode_current_letter),
                    marking_this_head, encode_current_letter.no_head,
                    target_head_move);
                // bottom head is in the same cell
                new_transition_(in_state, encode_current_letter.both_heads,
                                marking_this_head,
                                H::other_this_not(encode_current_letter),
                                target_head_move);

                const auto return_move =
                    target_head_move == HEAD_RIGHT ? HEAD_LEFT : HEAD_RIGHT;

                if (target_head_move == HEAD_RIGHT) {
                    new_transition_(
                        marking_this_head, BLANK, out_state,
                        H::this_other_not(
                            letters_map_[std::make_pair(BLANK, BLANK)]),
                        return_move);
                }

                new_transition_(
                    marking_this_head, encode_target_letter.no_head, out_state,
                    H::this_other_not(encode_target_letter), return_move);
                new_transition_(
                    marking_this_head, H::other_this_not(encode_target_letter),
                    out_state, encode_target_letter.both_heads, return_move);
            }
        }
    }

    void program_cleanup_();

    template <typename H>
    void program_look_for_(const Letter &this_letter, const State &in_state,
                           const State &out_state) {

        for (const Letter &other_letter : input_.working_alphabet()) {
            const auto letter_encoding =
                letters_map_[H::make_pair(this_letter, other_letter)];

            new_transition_(in_state, letter_encoding.both_heads, out_state,
                            letter_encoding.both_heads, HEAD_STAY);

            new_transition_(in_state, H::this_other_not(letter_encoding),
                            out_state, H::this_other_not(letter_encoding),
                            HEAD_STAY);
        }

        for (const auto &[letter_pair, letter_encoding] : letters_map_) {
            new_transition_(in_state, letter_encoding.no_head, in_state,
                            letter_encoding.no_head, HEAD_LEFT);

            new_transition_(in_state, H::other_this_not(letter_encoding),
                            in_state, H::other_this_not(letter_encoding),
                            HEAD_LEFT);
        }
    }

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
