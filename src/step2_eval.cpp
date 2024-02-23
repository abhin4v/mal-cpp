#include "evaluator.hpp"
#include "linenoise.h"
#include "printer.hpp"
#include "reader.hpp"
#include "types.hpp"
#include <iostream>
#include <ostream>
#include <sstream>

ExprPtr READ(const std::string &input) { return readStr(input); }

ExprPtr EVAL(const Env &replEnv, const ExprPtr &ast) {
  return eval(replEnv, ast);
}

std::string PRINT(const ExprPtr &value) { return prStr(value, true); }

std::string rep(const Env &replEnv, const std::string &input) {
  return PRINT(EVAL(replEnv, READ(input)));
}

static const auto HISTORY_FILE = ".repl_history";
static const auto HISTORY_MAX_LEN = 1000;

int main() {
  linenoiseHistoryLoad(HISTORY_FILE);
  linenoiseHistorySetMaxLen(HISTORY_MAX_LEN);

  auto replEnv = Env{
      {"+", {"add", [](auto arg1, auto arg2) { return arg1 + arg2; }}},
      {"-", {"sub", [](auto arg1, auto arg2) { return arg1 - arg2; }}},
      {"*", {"mult", [](auto arg1, auto arg2) { return arg1 * arg2; }}},
      {"/", {"div", [](auto arg1, auto arg2) { return arg1 / arg2; }}},
  };

  while (true) {
    auto *line = linenoise("user> ");
    if (line == nullptr) {
      break;
    }

    linenoiseHistoryAdd(line);          /* Add to the history. */
    linenoiseHistorySave(HISTORY_FILE); /* Save the history on disk. */

    try {
      auto output = rep(replEnv, std::string(line));
      std::cout << output << std::endl;
    } catch (const std::exception &e) {
      std::cerr << e.what() << std::endl;
    }

    linenoiseFree(line);
  }
}
