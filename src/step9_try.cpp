#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "constants.hpp"
#include "core.hpp"
#include "env.hpp"
#include "evaluator.hpp"
#include "linenoise.h"
#include "printer.hpp"
#include "reader.hpp"
#include "types.hpp"

Expr READ(const std::string &input) { return readStr(input); }

Expr EVAL(Evaluator &evaluator, const Expr &ast) { return evaluator.eval(ast); }

Expr trampoliningEVAL(Evaluator &evaluator, const Expr &ast) {
  return evaluator.trampoliningEval(ast);
}

std::string PRINT(const Expr &value) { return prStr(value, true); }

std::string rep(Evaluator &evaluator, const std::string &input) {
  return PRINT(EVAL(evaluator, READ(input)));
}

std::string trampoliningREP(Evaluator &evaluator, const std::string &input) {
  return PRINT(trampoliningEVAL(evaluator, READ(input)));
}

void setupREPL(const EnvPtr &replEnv, Evaluator &evaluator, const std::vector<std::string> &args) {
  for (const auto &[name, func] : Core::ns()) {
    replEnv->set(name, func);
  }

  replEnv->set("eval", makeBuiltinFunction<Expr>(
                           "eval", [&](const Expr &ast) { return EVAL(evaluator, ast); }));
  replEnv->set("trampolining-eval",
               makeBuiltinFunction<Expr>("trampolining-eval", [&](const Expr &ast) {
                 return trampoliningEVAL(evaluator, ast);
               }));
  replEnv->set(SWAP_BANG_FN_SYM,
               Core::makeSwapBangFunction([&](const Expr &ast) { return EVAL(evaluator, ast); }));

  rep(evaluator, Core::NOT_FUNC);
  rep(evaluator, Core::LOAD_FILE_FUNC);
  rep(evaluator, Core::COND_MACRO);

  ExprList progArgs{};
  for (auto i = 2; i < args.size(); i++) {
    progArgs = std::move(progArgs).push_back(String{args[i]});
  }
  replEnv->set("*ARGV*", List{progArgs});
}

static const auto HISTORY_FILE    = ".repl_history";
static const auto HISTORY_MAX_LEN = 1000;

void runREPL(Evaluator &evaluator) {
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
      auto output = rep(evaluator, std::string(line));
      std::cout << output << std::endl;
    } catch (const MALError &e) {
      std::cerr << e.what() << std::endl;
    } catch (const MALException &e) {
      std::cerr << "Exception: " << e.what() << std::endl;
    }

    linenoiseFree(line);
  }
}

int main(int argc, char *argv[]) {
  auto replEnv = std::make_shared<Env>(Env(nullptr));
  Evaluator evaluator(replEnv);
  std::vector<std::string> args(argv, argv + argc);
  setupREPL(replEnv, evaluator, args);

  if (args.size() > 1) {
    trampoliningREP(evaluator, "(load-file \"" + args[1] + "\")");
    return 0;
  }

  runREPL(evaluator);
}
