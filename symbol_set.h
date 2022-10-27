#include <string>
#include <unordered_set>

#include "turing_machine.h"

class SymbolSet {
  private:
    std::unordered_set<std::string> symbols;

  public:
    SymbolSet(const TuringMachine &tm);
    ~SymbolSet();

    std::string generate();

    std::string generate(const std::string &inspiration);
};
