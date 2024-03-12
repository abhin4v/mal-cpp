#include "evaluator.hpp"
#include "printer.hpp"
#include "types.hpp"
#include <iterator>

Expr Evaluator::trampoliningEval(const Expr &expr) {
  auto curExpr = expr;
  while (true) {
    auto result = doEval(curExpr);
    if (result.evaluated) {
      return result.result;
    }

    if (result.env != nullptr) {
      this->env = result.env;
    }

    curExpr = result.result;
  }
}

Expr Evaluator::eval(const Expr &expr) {
  auto result = doEval(expr);
  if (result.evaluated) {
    return result.result;
  }

  if (result.env == nullptr) {
    return eval(result.result);
  }

  Evaluator evaluator(result.env);
  return evaluator.eval(result.result);
}

EvalResult Evaluator::doEval(const Expr &expr) {
  static const std::unordered_set<std::string> SPECIAL_SYMBOLS = {
      "def!", "let*", "do", "if", "fn*",
  };

  auto firstSym = firstSymbolOfList(expr);
  if (firstSym.has_value() && SPECIAL_SYMBOLS.contains(*firstSym)) {
    return evalSpecialForm(*firstSym, std::get<Box<List>>(expr)->value);
  }

  auto evaluatedExpr = std::visit(*this, expr);
  if (const auto *list = std::get_if<Box<List>>(&evaluatedExpr)) {
    auto &elements = (*list)->value;
    if (!elements.empty()) {
      return applyFunction(elements);
    }
  }
  return EvalResult{true, nullptr, evaluatedExpr};
}

EvalResult Evaluator::evalSpecialForm(const std::string &specialSymbol,
                                      const ExprList &elements) {
  if (specialSymbol == "def!") {
    return EvalResult{true, env, evalDefBang(elements)};
  }
  if (specialSymbol == "let*") {
    return evalLetStar(elements);
  }
  if (specialSymbol == "do") {
    return evalDo(elements);
  }
  if (specialSymbol == "if") {
    return evalIf(elements);
  }
  if (specialSymbol == "fn*") {
    return EvalResult{true, env, evalFnStar(elements)};
  }

  throw std::runtime_error("not a special symbol: " + specialSymbol);
}

namespace {
Expr const *nth(const ExprList &exprs, size_t n) {
  auto iter = exprs.begin();
  std::advance(iter, n);
  return &*iter;
}
} // namespace

Expr Evaluator::evalDefBang(const ExprList &elements) {
  checkArgCount(elements.size() - 1, 2);
  return std::get<1>(
      defineBinding(*this, env, *nth(elements, 1), elements.back()));
}

EvalResult Evaluator::evalLetStar(const ExprList &elements) {
  checkArgCount(elements.size() - 1, 2);

  const auto &bindingsList = *nth(elements, 1);
  if (const auto *bindingList = std::get_if<Box<List>>(&bindingsList)) {
    return doEvalLetStar((*bindingList)->value, elements.back());
  }
  if (const auto *bindingList = std::get_if<Box<Vector>>(&bindingsList)) {
    return doEvalLetStar((*bindingList)->value, elements.back());
  }
  throw std::runtime_error("not a list or vector: " +
                           prStr(bindingsList, true));
}

template <typename Container>
EvalResult Evaluator::doEvalLetStar(const Container &bindingList,
                                    const Expr &letBody) {
  if (bindingList.empty()) {
    return EvalResult{false, env, letBody};
  }

  auto bindingListSize = bindingList.size();
  if (bindingListSize % 2 != 0) {
    throw std::runtime_error("odd number of elements in let* binding list");
  }

  Env letEnv(this->env);
  auto letEnvPtr = std::make_shared<Env>(letEnv);
  Evaluator letEvaluator(letEnvPtr);
  Evaluator *evaluator = this;

  std::unordered_map<std::string, Expr> bindings;
  bindings.reserve(bindingListSize / 2);

  for (auto iter = bindingList.begin(); iter != bindingList.end(); ++iter) {
    const auto bindingName = *iter;
    std::advance(iter, 1);
    auto nameValue = defineBinding(*evaluator, letEnvPtr, bindingName, *iter);
    bindings[nameValue.first] = nameValue.second;
    evaluator = &letEvaluator;
    letEnv = Env(letEnvPtr);
    letEnvPtr = std::make_shared<Env>(letEnv);
    letEvaluator = Evaluator(letEnvPtr);
  }

  for (const auto &[name, value] : bindings) {
    if (const auto *lambda = std::get_if<Box<Lambda>>(&value)) {
      // cppcheck-suppress shadowVariable
      for (const auto &[name, value] : bindings) {
        (*lambda)->env->set(name, value);
      }
    }
  }

  return EvalResult{false, evaluator->env, letBody};
}

EvalResult Evaluator::evalDo(const ExprList &elements) {
  if (elements.size() == 1) {
    return EvalResult{true, env, Nil::NIL};
  }

  // skip first and last element
  for (auto iter = std::next(elements.begin());
       iter != std::prev(elements.end()); ++iter) {
    eval(*iter);
  }

  return EvalResult{false, nullptr, elements.back()};
}

EvalResult Evaluator::evalIf(const ExprList &elements) {
  auto argCount = elements.size() - 1;
  if (argCount != 2 && argCount != 3) {
    throw std::runtime_error("expected two or three arguments, got: " +
                             std::to_string(argCount));
  }

  auto evaluatedCond = eval(*nth(elements, 1));
  if (const auto *cond = std::get_if<Bool>(&evaluatedCond)) {
    if (*cond == Bool::FALSE) {
      return EvalResult{argCount == 2, nullptr,
                        argCount == 3 ? elements.back() : Nil::NIL};
    }
  }
  if (const auto *cond = std::get_if<Nil>(&evaluatedCond)) {
    return EvalResult{argCount == 2, nullptr,
                      argCount == 3 ? elements.back() : Nil::NIL};
  }
  return EvalResult{false, nullptr, *nth(elements, 2)};
}

Expr Evaluator::evalFnStar(const ExprList &elements) {
  checkArgCount(elements.size() - 1, 2);

  const auto *mParams = nth(elements, 1);
  if (const auto *params = std::get_if<Box<List>>(mParams)) {
    return doEvalFnStar((*params)->value, elements.back());
  }
  if (const auto *params = std::get_if<Box<Vector>>(mParams)) {
    return doEvalFnStar((*params)->value, elements.back());
  }

  throw std::runtime_error("function params are not a list: " +
                           prStr(*mParams, true));
}
template <typename Container>
Expr Evaluator::doEvalFnStar(const Container &params, const Expr &body) {
  ParamNames paramNames;
  paramNames.reserve(params.size());
  bool variadic{false};
  int variadicParamsCount{0};
  for (const auto &param : params) {
    if (const auto *paramName = std::get_if<Symbol>(&param)) {
      if (paramName->value == "&") {
        variadic = true;
        continue;
      }
      if (variadic && variadicParamsCount == 1) {
        throw std::runtime_error(
            "more than one variadic function params specified");
      }

      paramNames.push_back(paramName->value);
      if (variadic) {
        variadicParamsCount++;
      }
      continue;
    }

    throw std::runtime_error("function param is not symbol: " +
                             prStr(param, true));
  }

  checkDupParamNames(paramNames);
  return Lambda{.params = std::move(paramNames),
                .variadic = variadic,
                .body = body,
                .env = std::make_shared<Env>(Env(env))};
}

EvalResult Evaluator::applyFunction(ExprList &calleeAndArgs) {
  const auto callee = std::move(calleeAndArgs.front());
  calleeAndArgs.pop_front();
  auto &args = calleeAndArgs;
  auto argCount = args.size();

  if (const auto *funcPtr = std::get_if<Box<BuiltinFunction>>(&callee)) {
    const auto &func = *funcPtr;
    if (func->arity != -1) {
      checkArgCount(argCount, func->arity);
    }
    return EvalResult{true, nullptr, func->value(args)};
  }

  if (const auto *lambdaPtr = std::get_if<Box<Lambda>>(&callee)) {
    const auto &lambda = *lambdaPtr;
    auto paramCount = lambda->params.size();

    if (lambda->variadic) {
      checkMinArgCount(argCount, paramCount - 1);
    } else {
      checkArgCount(argCount, paramCount);
    }

    auto lambdaEnv = std::make_shared<Env>(Env(lambda->env));
    ExprList variadicArgs{lambdaEnv->get("list").getRaw()};

    int idx = 0;
    for (const auto &arg : args) {
      if (lambda->variadic) {
        if (idx < paramCount - 1) {
          lambdaEnv->set(lambda->params[idx], arg);
        } else {
          variadicArgs.push_back(arg);
        }
      } else {
        lambdaEnv->set(lambda->params[idx], arg);
      }
      idx++;
    }

    if (lambda->variadic) {
      lambdaEnv->set(lambda->params[paramCount - 1], List{variadicArgs});
    }

    return EvalResult{false, lambdaEnv, lambda->body};
  }

  throw std::runtime_error("not a function: " + prStr(callee, true));
}
