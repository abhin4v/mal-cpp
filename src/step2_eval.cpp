#include "env.hpp"
#include "evaluator.hpp"
#include "linenoise.h"
#include "printer.hpp"
#include "reader.hpp"
#include "types.hpp"
#include <iostream>
#include <memory>
#include <ostream>
#include <sstream>

ExprPtr READ(const std::string &input) { return readStr(input); }

ExprPtr EVAL(Evaluator &evaluator, const ExprPtr &ast) {
  return evaluator.eval(ast);
}

std::string PRINT(const ExprPtr &value) { return prStr(value, true); }

std::string rep(Evaluator &evaluator, const std::string &input) {
  return PRINT(EVAL(evaluator, READ(input)));
}

static const auto HISTORY_FILE = ".repl_history";
static const auto HISTORY_MAX_LEN = 1000;

int main() {
  linenoiseHistoryLoad(HISTORY_FILE);
  linenoiseHistorySetMaxLen(HISTORY_MAX_LEN);

  Env replEnv(nullptr);
  replEnv.set("+", makeBuiltinFunction("add", [](auto arg1, auto arg2) {
                return arg1 + arg2;
              }));
  replEnv.set("-", makeBuiltinFunction("sub", [](auto arg1, auto arg2) {
                return arg1 - arg2;
              }));
  replEnv.set("*", makeBuiltinFunction("mult", [](auto arg1, auto arg2) {
                return arg1 * arg2;
              }));
  replEnv.set("/", makeBuiltinFunction("div", [](auto arg1, auto arg2) {
                return arg1 / arg2;
              }));

  Evaluator evaluator(std::make_shared<Env>(replEnv));

  while (true) {
    auto *line = linenoise("user> ");
    if (line == nullptr) {
      break;
    }

    linenoiseHistoryAdd(line);          /* Add to the history. */
    linenoiseHistorySave(HISTORY_FILE); /* Save the history on disk. */

    try {
      auto output = rep(evaluator, std::string(line));
      std::cout << output << std::endl;
    } catch (const std::exception &e) {
      std::cerr << e.what() << std::endl;
    }

    linenoiseFree(line);
  }
}
