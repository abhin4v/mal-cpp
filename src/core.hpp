#pragma once

#include "constants.hpp"
#include "equator.hpp"
#include "printer.hpp"
#include "reader.hpp"
#include "types.hpp"
#include <algorithm>
#include <fstream>
#include <functional>
#include <iostream>
#include <numeric>
#include <sstream>
#include <string>
#include <unordered_map>
#include <variant>

using Builtins = std::unordered_map<std::string, Expr>;

class Core {
private:
  static Builtins makeBuiltins() noexcept;

  static void makeArithmeticFunctions(Builtins &builtins);
  static void makeStringFunctions(Builtins &builtins);
  static void makePrintFunctions(Builtins &builtins);
  static void makeCollectionFunctions(Builtins &builtins);
  static void makeReadFunctions(Builtins &builtins);
  static void makeAtomFunctions(Builtins &builtins);
  static void makeTypePredicateFunctions(Builtins &builtins);

  static Expr apply(ExprList &args);
  static Expr map(const Expr &arg1, const Expr &arg2);
  static Expr nthElement(const Expr &arg1, const Expr &arg2);
  static Expr firstElement(const Expr &arg);
  static Expr restElements(const Expr &arg);

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
    if (std::holds_alternative<Nil>(arg)) {
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
    throw WrongTypeError("a list, vector or hashmap", arg);
  }

  static Expr concat(const ExprList &args) {
    if (args.empty()) {
      return List{{}};
    }

    ExprList newList;
    for (const auto &arg : args) {
      if (const auto *list = std::get_if<Box<List>>(&arg)) {
        newList.insert(newList.end(), (*list)->value.begin(),
                       (*list)->value.end());
        continue;
      }
      if (const auto *vec = std::get_if<Box<Vector>>(&arg)) {
        newList.insert(newList.end(), (*vec)->value.begin(),
                       (*vec)->value.end());
        continue;
      }
      throw WrongTypeError("a list or vector", arg);
    }

    return List{newList};
  }

  static void checkIndex(const size_t elementCount, const std::int64_t idx) {
    if (idx >= elementCount) {
      throw MALError("index " + std::to_string(idx) + " is out of bounds " +
                     std::to_string(elementCount));
    }
  }

  const inline static Builtins builtins = makeBuiltins();

public:
  static Builtins ns() { return builtins; }

  static Expr makeSwapBangFunction(std::function<Expr(const Expr &)> eval);

  static constexpr auto NOT_FUNC = "(def! not (fn* (a) (if a false true)))";
  static constexpr auto LOAD_FILE_FUNC =
      R"((def! load-file
                  (fn* (f) (trampolining-eval (read-string (str "(do " (slurp f) "\nnil) "))))))";
  static constexpr auto COND_MACRO =
      R"((defmacro! cond
                  (fn* (& xs)
                    (if (> (count xs) 0)
                      (list
                        'if
                        (first xs)
                        (if (> (count xs) 1)
                          (nth xs 1)
                          (throw "odd number of forms to cond"))
                        (cons 'cond (rest (rest xs))))))))";
};
