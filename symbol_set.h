#include <string>
#include <unordered_set>

#include "turing_machine.h"

class SymbolSet {
  private:
    std::unordered_set<std::string> symbols_;

    int counter_;

  public:
    SymbolSet(const std::vector<std::string> &symols);

    SymbolSet() = default;

    std::string generate();

    std::string generate(const std::string &inspiration);
};
