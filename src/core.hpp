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
#include <memory>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <string>
#include <variant>

class Core {
private:
  static std::unordered_map<std::string, Expr> makeBuiltins() noexcept;

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
    throw std::runtime_error("not a list, vector or hashmap: " +
                             prStr(arg, true));
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
      throw std::runtime_error("not a list or vector: " + prStr(arg, true));
    }

    return List{newList};
  }

  const inline static std::unordered_map<std::string, Expr> builtins =
      makeBuiltins();

public:
  static std::unordered_map<std::string, Expr> ns() { return builtins; }

  static Expr makeSwapBangFunction(std::function<Expr(const Expr &)> eval);

  static constexpr auto NOT_FUNC = "(def! not (fn* (a) (if a false true)))";
  static constexpr auto LOAD_FILE_FUNC =
      R"((def! load-file
                  (fn* (f) (trampolining-eval (read-string (str "(do " (slurp f) "\nnil) "))))))";
};
