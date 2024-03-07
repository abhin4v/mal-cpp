#pragma once

#include "env.hpp"
#include "printer.hpp"
#include "types.hpp"
#include <algorithm>
#include <iterator>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>

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
  Expr operator()(const Quote &expr) const { return expr; }
  Expr operator()(const QuasiQuote &expr) const { return expr; }
  Expr operator()(const Unquote &expr) const { return expr; }
  Expr operator()(const SpliceUnquote &expr) const { return expr; }
  Expr operator()(const Deref &expr) {
    auto evaledExpr = this->eval(expr.value);
    if (const auto *atom = std::get_if<Box<Atom>>(&evaledExpr)) {
      return (*atom)->value;
    }

    throw std::runtime_error("not an atom: " + prStr(expr.value, true));
  }
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

    ExprVec evaledElements;
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

  void evalElements(const ExprVec &elements, ExprVec &evaledElements) {
    const auto &that = this;
    evaledElements.reserve(elements.size());
    std::transform(elements.begin(), elements.end(),
                   std::back_inserter(evaledElements),
                   [&](const auto &elem) { return that->eval(elem); });
  }

  static std::optional<std::string> firstSymbolOfList(const Expr &expr) {
    if (const auto *list = std::get_if<Box<List>>(&expr)) {
      const auto &elements = (*list)->value;
      if (!elements.empty()) {
        // NOLINTNEXTLINE(readability-container-data-pointer)
        if (const auto *first = std::get_if<Symbol>(&elements[0])) {
          return std::make_optional(first->value);
        }
      }
    }
    return std::nullopt;
  }

  EvalResult evalSpecialForm(const std::string &specialSymbol,
                             const ExprVec &elements);

  Expr evalDefBang(const ExprVec &elements);

  EvalResult evalLetStar(const ExprVec &elements);

  EvalResult doEvalLetStar(const ExprVec &bindingList, const Expr &letBody);

  EvalResult evalDo(const ExprVec &elements);

  EvalResult evalIf(const ExprVec &elements);

  Expr evalFnStar(const ExprVec &elements);

  Expr doEvalFnStar(const ExprVec &params, const Expr &body);

  static EvalResult applyFunction(const ExprVec &calleeAndArgs);

  static std::pair<std::string, Expr> defineBinding(Evaluator &evaluator,
                                                    const EnvPtr &env,
                                                    const Expr &bindingNameExpr,
                                                    const Expr &bindingValue) {
    if (const auto *bindingName = std::get_if<Symbol>(&bindingNameExpr)) {
      auto evaluatedBindingValue = evaluator.eval(bindingValue);
      auto name = bindingName->value;
      env->set(name, evaluatedBindingValue);
      return {name, evaluatedBindingValue};
    }
    throw std::runtime_error("not a symbol: " + prStr(bindingNameExpr, true));
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
      throw std::runtime_error(error.str());
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
