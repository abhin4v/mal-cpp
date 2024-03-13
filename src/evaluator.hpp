#pragma once

#include "env.hpp"
#include "printer.hpp"
#include "types.hpp"
#include <algorithm>
#include <functional>
#include <iterator>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>
#include <variant>

struct EvalResult {
  bool evaluated;
  EnvPtr env;
  Expr result;
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
  Expr operator()(const MetaData &expr) { return this->eval(expr.value); }

  Expr operator()(const Symbol &sym) const {
    return env->get(sym.value).getRaw();
  }

  Expr operator()(const List &list) {
    auto elements = list.value;
    if (elements.empty()) {
      return list;
    }

    ExprList evaledElements;
    evalElements(elements, evaledElements);
    return List{evaledElements};
  }

  Expr operator()(const Vector &vector) {
    auto elements = vector.value;
    if (vector.value.empty()) {
      return vector;
    }

    ExprVec evaledElements;
    evalElements(elements, evaledElements);
    return Vector{evaledElements};
  }

  Expr operator()(const HashMap &hashmap) {
    if (hashmap.value.empty()) {
      return hashmap;
    }

    ExprMap map;
    for (const auto &[key, value] : hashmap.value) {
      map[eval(key)] = eval(value);
    }
    return HashMap{map};
  }

  Expr trampoliningEval(const Expr &expr);

  Expr eval(const Expr &expr);

private:
  EvalResult doEval(const Expr &expr);

  template <typename Container>
  void evalElements(const Container &elements, Container &evaledElements) {
    const auto &that = this;
    std::transform(elements.begin(), elements.end(),
                   std::back_inserter(evaledElements),
                   [&](const auto &elem) { return that->eval(elem); });
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

  EvalResult evalSpecialForm(const std::string &specialSymbol,
                             const ExprList &elements);

  Expr macroexpand(const Expr &expr);

  Expr evalDefBang(const ExprList &elements);

  Expr evalDefmacroBang(const ExprList &elements);

  EvalResult evalLetStar(const ExprList &elements);

  template <typename Container>
  EvalResult doEvalLetStar(const Container &bindingList, const Expr &letBody);

  EvalResult evalDo(const ExprList &elements);

  EvalResult evalIf(const ExprList &elements);

  Expr evalFnStar(const ExprList &elements);

  template <typename Container>
  Expr doEvalFnStar(const Container &params, const Expr &body);

  static Expr quasiquote(const Expr &expr);

  static Expr quasiquoteList(const List &list);

  static EvalResult applyFunction(ExprList &calleeAndArgs);

  std::optional<Box<Lambda>> findMacroFunc(const Expr &expr) {
    auto firstSym = firstSymbolOfList(expr);
    if (!firstSym.has_value()) {
      return std::nullopt;
    }

    const auto &val = env->find(firstSym.value());
    if (!val.has_value()) {
      return std::nullopt;
    }

    if (const auto *lambda = std::get_if<Box<Lambda>>(&val.value().getRaw())) {
      return std::make_optional(*lambda);
    }

    return std::nullopt;
  }

  static std::pair<std::string, Expr>
  defineBinding(const EnvPtr &env, const Expr &bindingNameExpr,
                const std::function<Expr()> &makeBindingValueExpr) {
    if (const auto *bindingName = std::get_if<Symbol>(&bindingNameExpr)) {
      auto evaluatedBindingValue = makeBindingValueExpr();
      auto name = bindingName->value;
      env->set(name, evaluatedBindingValue);
      return {name, evaluatedBindingValue};
    }
    throw WrongTypeError("a symbol", bindingNameExpr);
  }

  using ParamNames = std::vector<std::string>;

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
        duplicates.push_back(key);
      }
    }

    return duplicates;
  }
};
