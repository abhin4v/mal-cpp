#include "core.hpp"
#include "env.hpp"
#include "evaluator.hpp"
#include "linenoise.h"
#include "printer.hpp"
#include "reader.hpp"
#include "types.hpp"
#include <iostream>
#include <memory>
#include <numeric>
#include <ostream>
#include <sstream>

Expr READ(const std::string &input) { return readStr(input); }

Expr EVAL(Evaluator &evaluator, const Expr &ast) {
  return evaluator.trampoliningEval(ast);
}

std::string PRINT(const Expr &value) { return prStr(value, true); }

std::string rep(Evaluator &evaluator, const std::string &input) {
  return PRINT(EVAL(evaluator, READ(input)));
}

static const auto HISTORY_FILE = ".repl_history";
static const auto HISTORY_MAX_LEN = 1000;

int main() {
  linenoiseHistoryLoad(HISTORY_FILE);
  linenoiseHistorySetMaxLen(HISTORY_MAX_LEN);

  Env replEnv(nullptr);
  for (const auto &[name, func] : Core::ns()) {
    replEnv.set(name, func);
  }

  Evaluator evaluator(std::make_shared<Env>(replEnv));
  rep(evaluator, "(def! not (fn* (a) (if a false true)))");

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
