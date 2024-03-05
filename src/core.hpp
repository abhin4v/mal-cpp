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
  static std::unordered_map<std::string, ExprPtr> makeBuiltins() noexcept {
    std::unordered_map<std::string, ExprPtr> builtins;
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
    builtins.emplace(
        "=", makeBuiltinFunction("=", 2, [](const ExprPtrSpan &args) {
          return exprEquals(*args[0], *args[1]) ? truePtr() : falsePtr();
        }));
    builtins.emplace(
        "pr-str",
        makeBuiltinFunction("pr-str", -1, [](const ExprPtrSpan &args) {
          return std::make_shared<Expr>(String{toString(args, true, " ")});
        }));
    builtins.emplace(
        "str", makeBuiltinFunction("str", -1, [](const ExprPtrSpan &args) {
          return std::make_shared<Expr>(String{toString(args, false, "")});
        }));

    builtins.emplace(
        "prn", makeBuiltinFunction("prn", -1, [](const ExprPtrSpan &args) {
          std::cout << toString(args, true, " ") << std::endl;
          return nilPtr();
        }));
    builtins.emplace(
        "println",
        makeBuiltinFunction("println", -1, [](const ExprPtrSpan &args) {
          std::cout << toString(args, false, " ") << std::endl;
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
    builtins.emplace("read-string",
                     makeBuiltinFunction("read-string", [](const ExprPtr &arg) {
                       if (const auto *str = std::get_if<String>(&*arg)) {
                         return readStr(str->value);
                       }

                       throw std::runtime_error("not a string: " +
                                                prStr(arg, true));
                     }));
    builtins.emplace(
        "slurp", makeBuiltinFunction("slurp", [](const ExprPtr &arg) {
          if (const auto *str = std::get_if<String>(&*arg)) {
            const std::ifstream file(str->value);
            std::stringstream buffer;
            buffer << file.rdbuf();
            return std::make_shared<Expr>(String{buffer.str()});
          }

          throw std::runtime_error("not a string: " + prStr(arg, true));
        }));
    builtins.emplace("atom",
                     makeBuiltinFunction("atom", [](const ExprPtr &arg) {
                       return std::make_shared<Expr>(Atom{arg});
                     }));
    builtins.emplace("atom?",
                     makeBuiltinFunction("atom", [](const ExprPtr &arg) {
                       return std::holds_alternative<Atom>(*arg);
                     }));
    builtins.emplace(
        "deref", makeBuiltinFunction("deref", [](const ExprPtr &arg) {
          if (const auto *atom = std::get_if<Atom>(&*arg)) {
            return atom->value;
          }

          throw std::runtime_error("not an atom: " + prStr(arg, true));
        }));
    builtins.emplace(
        "reset!", makeBuiltinFunction("reset!", 2, [](const ExprPtrSpan &args) {
          if (auto *atom = std::get_if<Atom>(&*args[0])) {
            atom->value = args[1];
            return args[1];
          }

          throw std::runtime_error("not an atom: " + prStr(args[0], true));
        }));

    return builtins;
  }

  static std::string toString(const ExprPtrSpan &args, bool printReadably,
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
