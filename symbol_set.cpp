#include <sstream>

#include "symbol_set.h"

SymbolSet::SymbolSet(const std::vector<std::string> &symbols)
    : symbols_(symbols.begin(), symbols.end()), counter_(0) {}

std::string SymbolSet::generate() {
    std::stringstream ss;
    std::string res;

    do {
        ss << "(" << std::hex << ++counter_ << ")";
        ss >> res;
        ss.clear();
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

            ss << "(" << inspiration << std::hex << local_counter++ << ")";
            ss >> res;
            ss.clear();
        } while (symbols_.find(res) != symbols_.end());
        symbols_.insert(res);
        return res;
    }
}
