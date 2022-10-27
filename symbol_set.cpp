#include <sstream>

#include "symbol_set.h"

SymbolSet::SymbolSet(const std::vector<std::string> &symbols)
    : symbols_(symbols.begin(), symbols.end()) {}

std::string SymbolSet::generate() {
    std::stringstream ss;
    std::string res;

    do {
        ss << "(" << std::hex << counter_++ << ")";
        ss >> res;
    } while (symbols_.find(res) != symbols_.end());

    symbols_.insert(res);
    return res;
}

std::string SymbolSet::generate(const std::string &inspiration) {
    int local_counter = 0;

    std::string res = "(" + inspiration + ")";

    if (symbols_.find(res) == symbols_.end()) {
        symbols_.insert(res);
        return res;
    } else {
        std::stringstream ss;

        do {
            std::string hex;

            ss << std::hex << local_counter;

            res = "(" + inspiration + hex + ")";
        } while (symbols_.find(res) != symbols_.end());
        symbols_.insert(res);
        return res;
    }
}