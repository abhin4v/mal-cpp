#include "linenoise.h"
#include "printer.hpp"
#include "reader.hpp"
#include <iostream>
#include <ostream>
#include <sstream>

std::shared_ptr<Expr> READ(const std::string &input) { return readStr(input); }

std::shared_ptr<Expr> EVAL(const std::shared_ptr<Expr> &ast) { return ast; }

std::string PRINT(const std::shared_ptr<Expr> &value) {
  return prStr(value, true);
}

std::string rep(const std::string &input) { return PRINT(EVAL(READ(input))); }

static const auto HISTORY_FILE = ".repl_history";
static const auto HISTORY_MAX_LEN = 1000;

int main() {
  linenoiseHistoryLoad(HISTORY_FILE);
  linenoiseHistorySetMaxLen(HISTORY_MAX_LEN);

  while (true) {
    auto *line = linenoise("user> ");
    if (line == nullptr) {
      break;
    }

    linenoiseHistoryAdd(line);          /* Add to the history. */
    linenoiseHistorySave(HISTORY_FILE); /* Save the history on disk. */

    try {
      auto output = rep(std::string(line));
      std::cout << output << std::endl;
    } catch (const std::exception &e) {
      std::cerr << e.what() << std::endl;
    }

    linenoiseFree(line);
  }
}
