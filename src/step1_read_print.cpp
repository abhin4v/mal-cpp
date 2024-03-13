#include "linenoise.h"
#include "printer.hpp"
#include "reader.hpp"
#include <iostream>

Expr READ(const std::string &input) { return readStr(input); }

Expr EVAL(const Expr &ast) { return ast; }

std::string PRINT(const Expr &value) { return prStr(value, true); }

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
    } catch (const MALError &e) {
      std::cerr << e.what() << std::endl;
    }

    linenoiseFree(line);
  }
}
