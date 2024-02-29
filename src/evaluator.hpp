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

class Evaluator {
private:
  EnvPtr env;

public:
  explicit Evaluator(EnvPtr env) : env(std::move(env)) {}
  ExprPtr operator()(const Nil &nil) const {
    return std::make_shared<Expr>(nil);
  }
  ExprPtr operator()(const Int &num) const {
    return std::make_shared<Expr>(num);
  }
  ExprPtr operator()(const Bool &boolean) const {
    return std::make_shared<Expr>(boolean);
  }
  ExprPtr operator()(const String &str) const {
    return std::make_shared<Expr>(str);
  }
  ExprPtr operator()(const Keyword &kwd) const {
    return std::make_shared<Expr>(kwd);
  }
  ExprPtr operator()(const BuiltinFunction &func) const {
    return std::make_shared<Expr>(func);
  }
  ExprPtr operator()(const Lambda &lambda) const {
    return std::make_shared<Expr>(lambda);
  }
  ExprPtr operator()(const Quote &expr) const {
    return std::make_shared<Expr>(expr);
  }
  ExprPtr operator()(const QuasiQuote &expr) const {
    return std::make_shared<Expr>(expr);
  }
  ExprPtr operator()(const Unquote &expr) const {
    return std::make_shared<Expr>(expr);
  }
  ExprPtr operator()(const SpliceUnquote &expr) const {
    return std::make_shared<Expr>(expr);
  }
  ExprPtr operator()(const Deref &expr) const {
    return std::make_shared<Expr>(expr);
  }

  ExprPtr operator()(const MetaData &expr) {
    return std::visit(*this, *(expr.value));
  }

  ExprPtr operator()(const Symbol &sym) const { return env->get(sym.value); }

  ExprPtr operator()(const List &list) {
    auto elements = list.value;
    if (elements.empty()) {
      return std::make_shared<Expr>(list);
    }

    ExprPtrVec evaledElements;
    evalElements(elements, evaledElements);
    return std::make_shared<Expr>(List{evaledElements});
  }

  ExprPtr operator()(const Vector &vector) {
    auto elements = vector.value;
    if (vector.value.empty()) {
      return std::make_shared<Expr>(vector);
    }

    ExprPtrVec evaledElements;
    evalElements(elements, evaledElements);
    return std::make_shared<Expr>(Vector{evaledElements});
  }

  ExprPtr operator()(const HashMap &hashmap) {
    if (hashmap.value.empty()) {
      return std::make_shared<Expr>(hashmap);
    }

    ExprPtrMap map;
    for (const auto &[key, value] : hashmap.value) {
      map[eval(key)] = eval(value);
    }
    return std::make_shared<Expr>(HashMap{map});
  }

  [[nodiscard]] ExprPtr eval(const ExprPtr &exprPtr);

private:
  void evalElements(const ExprPtrVec &elements, ExprPtrVec &evaledElements) {
    evaledElements.reserve(elements.size());
    std::transform(elements.begin(), elements.end(),
                   std::back_inserter(evaledElements),
                   [&](const auto &elemPtr) { return eval(elemPtr); });
  }

  static std::optional<std::string> firstSymbolOfList(const ExprPtr &exprPtr) {
    if (const auto *list = std::get_if<List>(&*exprPtr)) {
      if (!list->value.empty()) {
        if (const auto *first = std::get_if<Symbol>(&*(list->value[0]))) {
          return std::make_optional(first->value);
        }
      }
    }
    return std::nullopt;
  }

  ExprPtr evalSpecialForm(const std::string &specialSymbol,
                          const ExprPtrVec &elements);

  ExprPtr evalDefBang(const ExprPtrVec &elements);

  ExprPtr evalLetStar(const ExprPtrVec &elements);

  ExprPtr doEvalLetStar(const ExprPtrVec &bindingList,
                        const ExprPtr &letBodyPtr);

  ExprPtr evalDo(const ExprPtrVec &elements);

  ExprPtr evalIf(const ExprPtrVec &elements);

  ExprPtr evalFnStar(const ExprPtrVec &elements);

  ExprPtr doEvalFnStar(const ExprPtrVec &params, const ExprPtr &body);

  ExprPtr callFunction(const ExprPtrVec &calleeAndArgs);

  static std::pair<std::string, ExprPtr>
  defineBinding(Evaluator &evaluator, const EnvPtr &env,
                const ExprPtr &bindingNamePtr, const ExprPtr &bindingValuePtr) {
    if (const auto *bindingName = std::get_if<Symbol>(&*bindingNamePtr)) {
      auto evaluatedBindingValue = evaluator.eval(bindingValuePtr);
      auto name = bindingName->value;
      env->set(name, evaluatedBindingValue);
      return {name, evaluatedBindingValue};
    }
    throw std::runtime_error("not a symbol: " + prStr(bindingNamePtr, true));
  }
};
