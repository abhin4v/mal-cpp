#pragma once

#include <exception>
#include <functional>
#include <immer/algorithm.hpp>
#include <iterator>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>

#include "env.hpp"
#include "printer.hpp"
#include "types.hpp"

struct EvalResult {
  bool evaluated;
  EnvPtr env;
  Expr result;
};

class MALException : public std::exception {
 private:
  Expr value;
  std::string message;

 public:
  explicit MALException(Expr value) : value(std::move(value)) {
    message = std::move(Printer::prStr(this->value, true));
  }

  [[nodiscard]] const char *what() const noexcept override { return message.c_str(); }

  [[nodiscard]] Expr getValue() const { return value; }
};

class Evaluator {
 private:
  EnvPtr env;

 public:
  explicit Evaluator(EnvPtr env) : env(std::move(env)) {}

  Expr operator()(const Nil &nil) const { return nil; }
  Expr operator()(const Int &num) const { return num; }
  Expr operator()(const Bool &boolean) const { return boolean; }
  Expr operator()(const String &str) const { return str; }
  Expr operator()(const Keyword &kwd) const { return kwd; }
  Expr operator()(const BuiltinFunction &func) const { return func; }
  Expr operator()(const Lambda &lambda) const { return lambda; }
  Expr operator()(const Atom &expr) const { return expr; }
  Expr operator()(const Symbol &sym) const { return env->get(sym.value).getRaw(); }
  Expr operator()(const List &list) { return List{evalElements(list.value)}; }
  Expr operator()(const Vector &vector) { return Vector{evalElements(vector.value)}; }

  Expr operator()(const HashMap &hashmap) {
    ExprMap map;
    for (const auto &[key, value] : hashmap) {
      map = std::move(map).set(eval(key), eval(value));
    }
    return HashMap{map};
  }

  Expr eval(const Expr &expr);
  Expr trampoliningEval(const Expr &expr);
  static Expr apply(const Expr &callee, const ExprList &args);

 private:
  EvalResult doEval(const Expr &expr);

  EvalResult evalSpecialForm(const std::string &specialSymbol, const ExprList &args);

  Expr evalDefBang(const ExprList &args);

  Expr evalDefmacroBang(const ExprList &args);

  EvalResult evalLetStar(const ExprList &args);

  template <typename Container>
  EvalResult doEvalLetStar(const Container &bindingList, const Expr &letBody);

  EvalResult evalDo(const ExprList &args);

  EvalResult evalIf(const ExprList &args);

  Expr evalFnStar(const ExprList &args);

  template <typename Container>
  Expr doEvalFnStar(const Container &params, const Expr &body);

  Expr macroexpand(const Expr &expr);

  static Expr quasiquote(const Expr &expr);

  static Expr quasiquoteList(const List &list);

  Expr evalTryStar(const ExprList &args);

  static EvalResult applyFunction(const Expr &callee, const ExprList &args);

  std::optional<Box<Lambda>> findMacroFunc(const Expr &expr);

  static std::pair<std::string, Expr> defineBinding(
      const EnvPtr &env, const Expr &bindingNameExpr,
      const std::function<Expr()> &makeBindingValueExpr);

  template <typename Container>
  Container evalElements(const Container &elements) {
    return immer::accumulate(elements.begin(), elements.end(), Container{},
                             [&](auto acc, const auto &elem) { return acc.push_back(eval(elem)); });
  }

  static std::optional<std::string> firstSymbolOfList(const Expr &expr) {
    if (const auto *list = std::get_if<Box<List>>(&expr)) {
      const auto &elements = (*list)->value;
      if (!elements.empty()) {
        // NOLINTNEXTLINE(readability-container-data-pointer)
        if (const auto *first = std::get_if<Symbol>(&elements.front())) {
          return std::make_optional(first->value);
        }
      }
    }
    return std::nullopt;
  }

  static void checkDupParamNames(const ParamNames &paramNames) {
    auto dupParamNames = findDuplicates(paramNames);
    if (!dupParamNames.empty()) {
      std::stringstream error;
      error << "duplicate parameter names: ";
      for (const auto &param : dupParamNames) {
        error << param << " ";
      }
      throw MALError(error.str());
    }
  }

  static ParamNames findDuplicates(const ParamNames &paramNames) {
    std::unordered_map<std::string, int> counts;
    ParamNames duplicates;

    for (const auto &word : paramNames) {
      ++counts[word];
    }

    for (const auto &[key, count] : counts) {
      if (count > 1) {
        duplicates = std::move(duplicates).push_back(key);
      }
    }

    return duplicates;
  }
};
