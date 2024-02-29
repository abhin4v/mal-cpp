#pragma once

#include "equator.hpp"
#include "printer.hpp"
#include "types.hpp"
#include <iostream>
#include <memory>
#include <numeric>
#include <variant>

class Core {
private:
  static std::unordered_map<std::string, ExprPtr> makeBuiltins() noexcept {
    std::unordered_map<std::string, ExprPtr> builtins;
    builtins.emplace(
        "+",
        makeBuiltinFunction("add", [](const std::vector<std::int64_t> &args) {
          auto ret = std::accumulate(args.begin(), args.end(), 0LL);
          return ret;
        }));
    builtins.emplace(
        "*",
        makeBuiltinFunction("mult", [](const std::vector<std::int64_t> &args) {
          return std::accumulate(args.begin(), args.end(), 1LL,
                                 std::multiplies<>());
        }));
    builtins.emplace("-", makeBuiltinFunction("sub", [](auto arg1, auto arg2) {
                       return arg1 - arg2;
                     }));
    builtins.emplace("/", makeBuiltinFunction("div", [](auto arg1, auto arg2) {
                       return arg1 / arg2;
                     }));
    builtins.emplace(
        "<", makeBuiltinFunction("<", 2, [](const ExprPtrSpan &args) {
          return intVal(args[0]) < intVal(args[1]) ? truePtr() : falsePtr();
        }));
    builtins.emplace(
        ">", makeBuiltinFunction(">", 2, [](const ExprPtrSpan &args) {
          return intVal(args[0]) > intVal(args[1]) ? truePtr() : falsePtr();
        }));
    builtins.emplace(
        "<=", makeBuiltinFunction("<=", 2, [](const ExprPtrSpan &args) {
          return intVal(args[0]) <= intVal(args[1]) ? truePtr() : falsePtr();
        }));
    builtins.emplace(
        ">=", makeBuiltinFunction(">=", 2, [](const ExprPtrSpan &args) {
          return intVal(args[0]) >= intVal(args[1]) ? truePtr() : falsePtr();
        }));
    builtins.emplace("prn", makeBuiltinFunction("prn", [](const ExprPtr &arg) {
                       std::cout << prStr(arg, true) << std::endl;
                       return nilPtr();
                     }));
    builtins.emplace("list?",
                     makeBuiltinFunction("list?", [](const ExprPtr &arg) {
                       return std::holds_alternative<List>(*arg);
                     }));
    builtins.emplace("empty?",
                     makeBuiltinFunction("empty?", [](const ExprPtr &arg) {
                       return collectionCount(arg) == 0;
                     }));
    builtins.emplace("count",
                     makeBuiltinFunction("count", [](const ExprPtr &arg) {
                       return std::make_shared<Expr>(Int{collectionCount(arg)});
                     }));
    builtins.emplace(
        "list", makeBuiltinFunction("list", -1, [](const ExprPtrSpan &args) {
          return std::make_shared<Expr>(List{{args.begin(), args.end()}});
        }));
    builtins.emplace(
        "=", makeBuiltinFunction("=", 2, [](const ExprPtrSpan &args) {
          return exprEquals(*args[0], *args[1]) ? truePtr() : falsePtr();
        }));

    return builtins;
  }

  static std::int64_t collectionCount(const ExprPtr &arg) {
    if (const auto *nil = std::get_if<Nil>(&*arg)) {
      return 0;
    }
    if (const auto *list = std::get_if<List>(&*arg)) {
      return static_cast<std::int64_t>(list->value.size());
    }
    if (const auto *vector = std::get_if<Vector>(&*arg)) {
      return static_cast<std::int64_t>(vector->value.size());
    }
    if (const auto *hashmap = std::get_if<HashMap>(&*arg)) {
      return static_cast<std::int64_t>(hashmap->value.size());
    }
    throw std::runtime_error("not a list, vector or hashmap: " +
                             prStr(arg, true));
  }

  const inline static std::unordered_map<std::string, ExprPtr> builtins =
      makeBuiltins();

public:
  static std::unordered_map<std::string, ExprPtr> ns() { return builtins; }
};
