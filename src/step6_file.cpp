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
#include <vector>

ExprPtr READ(const std::string &input) { return readStr(input); }

ExprPtr EVAL(Evaluator &evaluator, const ExprPtr &ast) {
  return evaluator.eval(ast);
}

ExprPtr trampoliningEVAL(Evaluator &evaluator, const ExprPtr &ast) {
  return evaluator.trampoliningEval(ast);
}

std::string PRINT(const ExprPtr &value) { return prStr(value, true); }

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

  replEnv->set("eval", makeBuiltinFunction("eval", [&](const ExprPtr &ast) {
                 return EVAL(evaluator, ast);
               }));
  replEnv->set(
      "trampolining-eval",
      makeBuiltinFunction("trampolining-eval", [&](const ExprPtr &ast) {
        return trampoliningEVAL(evaluator, ast);
      }));
  replEnv->set(
      "swap!", makeBuiltinFunction("swap!", -1, [&](const ExprPtrSpan &args) {
        checkMinArgCount(args.size(), 2);
        if (auto *atom = std::get_if<Atom>(&*args[0])) {
          ExprPtrVec funcAndArgs;
          funcAndArgs.reserve(args.size());
          funcAndArgs.push_back(args[1]);
          funcAndArgs.push_back(atom->value);
          for (const auto &arg : args.subspan(2)) {
            funcAndArgs.push_back(arg);
          }

          auto newValue =
              EVAL(evaluator, std::make_shared<Expr>(List{funcAndArgs}));
          atom->value = newValue;
          return newValue;
        }

        throw std::runtime_error("not an atom: " + prStr(args[0], true));
      }));

  rep(evaluator, "(def! not (fn* (a) (if a false true)))");
  rep(evaluator,
      R"((def! load-file
                  (fn* (f) (trampolining-eval (read-string (str "(do " (slurp f) "\nnil) "))))))");

  ExprPtrVec progArgs{replEnv->get("list")};
  for (auto i = 2; i < args.size(); i++) {
    progArgs.emplace_back(std::make_shared<Expr>(String{args[i]}));
  }
  replEnv->set("*ARGV*", std::make_shared<Expr>(List{progArgs}));
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
