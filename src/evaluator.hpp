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

  static ExprPtr defineBinding(Evaluator &evaluator, const EnvPtr &env,
                               const ExprPtr &bindingNamePtr,
                               const ExprPtr &bindingValuePtr) {
    if (const auto *bindingName = std::get_if<Symbol>(&*bindingNamePtr)) {
      auto evaluatedBindingValue = evaluator.eval(bindingValuePtr);
      auto name = bindingName->value;
      env->set(std::move(name), evaluatedBindingValue);
      return evaluatedBindingValue;
    }
    throw std::runtime_error("not a symbol: " + prStr(bindingNamePtr, true));
  }

  static ExprPtr callFunction(const ExprPtrVec &calleeAndArgs) {
    const auto &callee = calleeAndArgs[0];
    if (const auto *func = std::get_if<BuiltinFunction>(&*callee)) {
      checkParamCount(calleeAndArgs.size() - 1, 2);

      return std::make_shared<Expr>(
          Int{func->value(intVal(calleeAndArgs[1]), intVal(calleeAndArgs[2]))});
    }

    throw std::runtime_error("not a function: " + prStr(callee, true));
  }

  static void checkParamCount(size_t paramCount, size_t expected) {
    if (paramCount != expected) {
      throw std::runtime_error(
          "expected " + std::to_string(expected) +
          " arguments, got: " + std::to_string(paramCount));
    }
  }

  static std::int64_t intVal(const ExprPtr &exprPtr) {
    if (const auto *expr = std::get_if<Int>(&*exprPtr)) {
      return expr->value;
    }
    throw std::runtime_error("not an integer: " + prStr(exprPtr, true));
  }
};
