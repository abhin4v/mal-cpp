#pragma once

#include "equator.hpp"
#include "printer.hpp"
#include "reader.hpp"
#include "types.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <memory>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <string>
#include <variant>

class Core {
private:
  static std::unordered_map<std::string, Expr> makeBuiltins() noexcept {
    std::unordered_map<std::string, Expr> builtins;
    builtins.emplace(
        "+",
        makeBuiltinFunction("add", [](const std::vector<std::int64_t> &args) {
          return std::accumulate(args.begin(), args.end(), 0LL);
        }));
    builtins.emplace(
        "*",
        makeBuiltinFunction("mult", [](const std::vector<std::int64_t> &args) {
          return std::accumulate(args.begin(), args.end(), 1LL,
                                 std::multiplies<>());
        }));
    builtins.emplace("-", makeBuiltinFunction<std::int64_t>(
                              "sub", [](std::int64_t arg1, std::int64_t arg2) {
                                return arg1 - arg2;
                              }));
    builtins.emplace("/", makeBuiltinFunction<std::int64_t>(
                              "div", [](std::int64_t arg1, std::int64_t arg2) {
                                return arg1 / arg2;
                              }));
    builtins.emplace("<", makeBuiltinFunction<bool>(
                              "<", [](std::int64_t arg1, std::int64_t arg2) {
                                return arg1 < arg2;
                              }));
    builtins.emplace(">", makeBuiltinFunction<bool>(
                              ">", [](std::int64_t arg1, std::int64_t arg2) {
                                return arg1 > arg2;
                              }));
    builtins.emplace("<=", makeBuiltinFunction<bool>(
                               "<=", [](std::int64_t arg1, std::int64_t arg2) {
                                 return arg1 <= arg2;
                               }));
    builtins.emplace(">=", makeBuiltinFunction<bool>(
                               ">=", [](std::int64_t arg1, std::int64_t arg2) {
                                 return arg1 >= arg2;
                               }));
    builtins.emplace("=", makeBuiltinFunction<bool>(
                              "=", [](const Expr &arg1, const Expr &arg2) {
                                return exprEquals(arg1, arg2);
                              }));
    builtins.emplace(
        "pr-str", makeBuiltinFunction("pr-str", -1, [](const ExprList &args) {
          return String{toString(args, true, " ")};
        }));
    builtins.emplace("str",
                     makeBuiltinFunction("str", -1, [](const ExprList &args) {
                       return String{toString(args, false, "")};
                     }));

    builtins.emplace("prn",
                     makeBuiltinFunction("prn", -1, [](const ExprList &args) {
                       std::cout << toString(args, true, " ") << std::endl;
                       return Nil::NIL;
                     }));
    builtins.emplace(
        "println", makeBuiltinFunction("println", -1, [](const ExprList &args) {
          std::cout << toString(args, false, " ") << std::endl;
          return Nil::NIL;
        }));
    builtins.emplace("list?",
                     makeBuiltinFunction<bool>("list?", [](const Expr &arg) {
                       return std::holds_alternative<Box<List>>(arg);
                     }));
    builtins.emplace("empty?",
                     makeBuiltinFunction<bool>("empty?", [](const Expr &arg) {
                       return collectionCount(arg) == 0;
                     }));
    builtins.emplace("count",
                     makeBuiltinFunction<Expr>("count", [](const Expr &arg) {
                       return Int{collectionCount(arg)};
                     }));
    builtins.emplace("list",
                     makeBuiltinFunction("list", -1, [](const ExprList &args) {
                       return List{args};
                     }));
    builtins.emplace(
        "read-string",
        makeBuiltinFunction<Expr>("read-string", [](const Expr &arg) {
          if (const auto *str = std::get_if<String>(&arg)) {
            return readStr(str->value);
          }

          throw std::runtime_error("not a string: " + prStr(arg, true));
        }));
    builtins.emplace(
        "slurp", makeBuiltinFunction<Expr>("slurp", [](const Expr &arg) {
          if (const auto *str = std::get_if<String>(&arg)) {
            const std::ifstream file(str->value);
            std::stringstream buffer;
            buffer << file.rdbuf();
            return String{buffer.str()};
          }

          throw std::runtime_error("not a string: " + prStr(arg, true));
        }));
    builtins.emplace("atom",
                     makeBuiltinFunction<Expr>(
                         "atom", [](const Expr &arg) { return Atom{arg}; }));
    builtins.emplace("atom?",
                     makeBuiltinFunction<bool>("atom?", [](const Expr &arg) {
                       return std::holds_alternative<Box<Atom>>(arg);
                     }));
    builtins.emplace(
        "deref", makeBuiltinFunction<Expr>("deref", [](const Expr &arg) {
          if (const auto *atom = std::get_if<Box<Atom>>(&arg)) {
            return (*atom)->value;
          }

          throw std::runtime_error("not an atom: " + prStr(arg, true));
        }));
    builtins.emplace(
        "reset!", makeBuiltinFunction<Expr>("reset!", [](const Expr &arg1,
                                                         const Expr &arg2) {
          if (const auto *atom = std::get_if<Box<Atom>>(&arg1)) {
            (*atom)->value = arg2;
            return arg2;
          }

          throw std::runtime_error("not an atom: " + prStr(arg1, true));
        }));

    return builtins;
  }

  static std::string toString(const ExprList &args, bool printReadably,
                              const std::string &separator) {
    if (args.empty()) {
      return "";
    }
    std::stringstream buffer;
    std::for_each(args.begin(), args.end(), [&](const auto &arg) {
      buffer << prStr(arg, printReadably) << separator;
    });
    auto str{buffer.str()};
    return str.substr(0, str.size() - separator.size());
  }

  static std::int64_t collectionCount(const Expr &arg) {
    if (const auto *nil = std::get_if<Nil>(&arg)) {
      return 0;
    }
    if (const auto *list = std::get_if<Box<List>>(&arg)) {
      return static_cast<std::int64_t>((*list)->value.size());
    }
    if (const auto *vector = std::get_if<Box<Vector>>(&arg)) {
      return static_cast<std::int64_t>((*vector)->value.size());
    }
    if (const auto *hashmap = std::get_if<Box<HashMap>>(&arg)) {
      return static_cast<std::int64_t>((*hashmap)->value.size());
    }
    throw std::runtime_error("not a list, vector or hashmap: " +
                             prStr(arg, true));
  }

  const inline static std::unordered_map<std::string, Expr> builtins =
      makeBuiltins();

public:
  static std::unordered_map<std::string, Expr> ns() { return builtins; }
};
