#pragma once

#include <algorithm>
#include <fstream>
#include <functional>
#include <iostream>
#include <numeric>
#include <sstream>
#include <string>
#include <variant>

#include "constants.hpp"
#include "env.hpp"
#include "equator.hpp"
#include "printer.hpp"
#include "reader.hpp"
#include "types.hpp"

class Core {
 public:
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

  static void install(const EnvPtr &env);
  static Expr makeWithMetaFunction(const EnvPtr &env);
  static Expr makeSwapBangFunction(std::function<Expr(const Expr &)> eval);

 private:
  static void installArithmeticFunctions(const EnvPtr &env);
  static void installStringFunctions(const EnvPtr &env);
  static void installPrintFunctions(const EnvPtr &env);
  static void installCollectionFunctions(const EnvPtr &env);
  static void installReadFunctions(const EnvPtr &env);
  static void installAtomFunctions(const EnvPtr &env);
  static void installTypePredicateFunctions(const EnvPtr &env);

  static Expr readline(const String &str);
  static Expr apply(const ExprList &args);
  static Expr map(const Expr &arg1, const Expr &arg2);
  static std::int64_t collectionCount(const Expr &arg);
  static Expr concat(const ExprList &args);
  static Expr nthElement(const Expr &arg1, const Expr &arg2);
  static Expr firstElement(const Expr &arg);
  static Expr restElements(const Expr &arg);
  static Expr seq(const Expr &arg);
  static Expr conj(const ExprList &args);
  static Expr assoc(const ExprList &args);
  static Expr dissoc(const ExprList &args);
  static Expr get(const Expr &arg1, const Expr &arg2);
  static Expr keys(const Box<HashMap> &hashmap);
  static Expr vals(const Box<HashMap> &hashmap);
  static Expr meta(const Expr &arg);

  static void installFunc(const EnvPtr &env, BuiltinFunction func) {
    auto name = func.name;
    env->set(name, std::move(func));
  }

  static std::string toString(const ExprList &args, bool printReadably,
                              const std::string &separator) {
    if (args.empty()) {
      return "";
    }
    std::stringstream buffer;
    std::for_each(args.begin(), args.end(), [&](const auto &arg) {
      buffer << Printer::prStr(arg, printReadably) << separator;
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
};
