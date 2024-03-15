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

  static Expr readline(const String &str);
  static Expr apply(ExprList &args);
  static Expr map(const Expr &arg1, const Expr &arg2);
  static std::int64_t collectionCount(const Expr &arg);
  static Expr concat(const ExprList &args);
  static Expr nthElement(const Expr &arg1, const Expr &arg2);
  static Expr firstElement(const Expr &arg);
  static Expr restElements(const Expr &arg);
  static Expr seq(const Expr &arg);
  static Expr conj(ExprList &args);
  static Expr assoc(ExprList &args);
  static Expr dissoc(ExprList &args);
  static Expr get(const Expr &arg1, const Expr &arg2);
  static Expr keys(const Box<HashMap> &hashmap);
  static Expr vals(const Box<HashMap> &hashmap);

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
