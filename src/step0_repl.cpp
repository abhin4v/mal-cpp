#include "linenoise.h"
#include <iostream>
#include <string>

std::string READ(const std::string &input) { return input; }

std::string EVAL(const std::string &ast) { return ast; }

std::string PRINT(const std::string &value) { return value; }

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

    auto output = rep(std::string(line));
    std::cout << output << std::endl;

    linenoiseFree(line);
  }
}
