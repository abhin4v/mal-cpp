#pragma once

#include "env.hpp"
#include "printer.hpp"
#include "types.hpp"
#include <algorithm>
#include <iterator>
#include <memory>
#include <utility>

class Evaluator {
private:
  Env env;

public:
  explicit Evaluator(Env env) : env(std::move(env)) {}
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

  ExprPtr operator()(const MetaData &expr) const {
    return std::visit(*this, *(expr.value));
  }

  ExprPtr operator()(const Symbol &sym) const { return env.get(sym.value); }

  ExprPtr operator()(const List &list) const {
    auto elements = list.value;
    if (elements.empty()) {
      return std::make_shared<Expr>(list);
    }

    ExprPtrVec evaledElements;
    evaledElements.reserve(elements.size());
    std::transform(elements.begin(), elements.end(),
                   std::back_inserter(evaledElements),
                   [&](const auto &elemPtr) { return eval(elemPtr); });
    return std::make_shared<Expr>(List{evaledElements});
  }

  ExprPtr operator()(const Vector &vector) const {
    auto elements = vector.value;
    if (vector.value.empty()) {
      return std::make_shared<Expr>(vector);
    }

    ExprPtrVec evaledElements;
    elements.reserve(elements.size());
    std::transform(elements.begin(), elements.end(),
                   std::back_inserter(evaledElements),
                   [&](const auto &elemPtr) { return eval(elemPtr); });

    return std::make_shared<Expr>(Vector{evaledElements});
  }

  ExprPtr operator()(const HashMap &hashmap) const {
    if (hashmap.value.empty()) {
      return std::make_shared<Expr>(hashmap);
    }

    ExprPtrMap map;
    for (const auto &[key, value] : hashmap.value) {
      map[eval(key)] = eval(value);
    }
    return std::make_shared<Expr>(HashMap{map});
  }

  [[nodiscard]] ExprPtr eval(const ExprPtr &exprPtr) const {
    auto evaluatedExprPtr = std::visit(*this, *exprPtr);

    if (const auto *list = std::get_if<List>(&*evaluatedExprPtr)) {
      if (!list->value.empty()) {
        return callFunction(list->value);
      }
    }
    return evaluatedExprPtr;
  }

private:
  static std::int64_t intVal(const ExprPtr &exprPtr) {
    if (const auto *expr = std::get_if<Int>(&*exprPtr)) {
      return expr->value;
    }
    throw std::runtime_error("not an integer: " + prStr(exprPtr, true));
  }

  static ExprPtr callFunction(const ExprPtrVec &calleeAndArgs) {
    const auto &callee = calleeAndArgs[0];
    if (const auto *func = std::get_if<BuiltinFunction>(&*callee)) {
      auto argCount = calleeAndArgs.size() - 1;
      if (argCount != 2) {
        throw std::runtime_error("expected two arguments, got: " +
                                 std::to_string(argCount));
      }

      return std::make_shared<Expr>(
          Int{func->value(intVal(calleeAndArgs[1]), intVal(calleeAndArgs[2]))});
    }

    throw std::runtime_error("not a function: " + prStr(callee, true));
  }
};
