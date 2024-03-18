#include <iostream>
#include <memory>
#include <string>

#include "core.hpp"
#include "env.hpp"
#include "evaluator.hpp"
#include "linenoise.h"
#include "printer.hpp"
#include "reader.hpp"
#include "types.hpp"

Expr READ(const std::string &input) { return readStr(input); }

Expr EVAL(Evaluator &evaluator, const Expr &ast) { return evaluator.eval(ast); }

std::string PRINT(const Expr &value) { return Printer::prStr(value, true); }

std::string rep(Evaluator &evaluator, const std::string &input) {
  return PRINT(EVAL(evaluator, READ(input)));
}

static const auto HISTORY_FILE    = ".repl_history";
static const auto HISTORY_MAX_LEN = 1000;

int main() {
  linenoiseHistoryLoad(HISTORY_FILE);
  linenoiseHistorySetMaxLen(HISTORY_MAX_LEN);

  const Env replEnv(nullptr);
  auto replEnvPtr = std::make_shared<Env>(replEnv);
  Core::install(replEnvPtr);

  Evaluator evaluator(replEnvPtr);

  while (true) {
    auto *line = linenoise("user> ");
    if (line == nullptr) {
      break;
    }

    linenoiseHistoryAdd(line);
    linenoiseHistorySave(HISTORY_FILE);

    try {
      auto output = rep(evaluator, std::string(line));
      std::cout << output << std::endl;
    } catch (const MALError &e) {
      std::cerr << e.what() << std::endl;
    }

    linenoiseFree(line);
  }
}
