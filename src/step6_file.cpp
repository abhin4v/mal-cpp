#include "core.hpp"
#include "env.hpp"
#include "evaluator.hpp"
#include "linenoise.h"
#include "printer.hpp"
#include "reader.hpp"
#include "types.hpp"
#include <algorithm>
#include <iostream>
#include <iterator>
#include <memory>
#include <numeric>
#include <ostream>
#include <sstream>
#include <vector>

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

void setupREPL(const EnvPtr &replEnv, Evaluator &evaluator,
               const std::vector<std::string> &args) {
  for (const auto &[name, func] : Core::ns()) {
    replEnv->set(name, func);
  }

  replEnv->set("eval", makeBuiltinFunction("eval", [&](const Expr &ast) {
                 return EVAL(evaluator, ast);
               }));
  replEnv->set("trampolining-eval",
               makeBuiltinFunction("trampolining-eval", [&](const Expr &ast) {
                 return trampoliningEVAL(evaluator, ast);
               }));
  replEnv->set("swap!",
               makeBuiltinFunction("swap!", -1, [&](const ExprSpan &args) {
                 checkMinArgCount(args.size(), 2);
                 const auto &arg = args[0];
                 if (const auto *atom = std::get_if<Box<Atom>>(&arg)) {
                   ExprVec funcAndArgs;
                   funcAndArgs.reserve(args.size());
                   funcAndArgs.push_back(args[1]);
                   funcAndArgs.push_back((*atom)->value);
                   auto extraArgs = args.subspan(2);
                   std::copy(extraArgs.begin(), extraArgs.end(),
                             std::back_insert_iterator(funcAndArgs));

                   auto newValue = EVAL(evaluator, List{funcAndArgs});
                   (*atom)->value = newValue;
                   return newValue;
                 }

                 throw std::runtime_error("not an atom: " + prStr(arg, true));
               }));

  rep(evaluator, "(def! not (fn* (a) (if a false true)))");
  rep(evaluator,
      R"((def! load-file
                  (fn* (f) (trampolining-eval (read-string (str "(do " (slurp f) "\nnil) "))))))");

  ExprVec progArgs{replEnv->get("list").getRaw()};
  for (auto i = 2; i < args.size(); i++) {
    progArgs.emplace_back(String{args[i]});
  }
  replEnv->set("*ARGV*", List{progArgs});
}

static const auto HISTORY_FILE = ".repl_history";
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
    } catch (const std::exception &e) {
      std::cerr << e.what() << std::endl;
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
