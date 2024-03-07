#include "evaluator.hpp"
#include "printer.hpp"
#include "types.hpp"

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
    const auto &elements = (*list)->value;
    if (!elements.empty()) {
      return applyFunction(elements);
    }
  }
  return EvalResult{true, nullptr, evaluatedExpr};
}

EvalResult Evaluator::evalSpecialForm(const std::string &specialSymbol,
                                      const ExprVec &elements) {
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

Expr Evaluator::evalDefBang(const ExprVec &elements) {
  checkArgCount(elements.size() - 1, 2);
  return std::get<1>(defineBinding(*this, env, elements[1], elements[2]));
}

EvalResult Evaluator::evalLetStar(const ExprVec &elements) {
  checkArgCount(elements.size() - 1, 2);

  const auto &bindingsList = elements[1];
  if (const auto *bindingList = std::get_if<Box<List>>(&bindingsList)) {
    return doEvalLetStar((*bindingList)->value, elements[2]);
  }
  if (const auto *bindingList = std::get_if<Box<Vector>>(&bindingsList)) {
    return doEvalLetStar((*bindingList)->value, elements[2]);
  }
  throw std::runtime_error("not a list or vector: " +
                           prStr(bindingsList, true));
}

EvalResult Evaluator::doEvalLetStar(const ExprVec &bindingList,
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

  for (auto i = 0; i < bindingListSize; i += 2) {
    auto nameValue = defineBinding(*evaluator, letEnvPtr, bindingList[i],
                                   bindingList[i + 1]);
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

EvalResult Evaluator::evalDo(const ExprVec &elements) {
  if (elements.size() == 1) {
    return EvalResult{true, env, Nil::NIL};
  }

  for (auto i = 1; i < elements.size() - 1; i++) {
    eval(elements[i]);
  }

  return EvalResult{false, nullptr, elements.back()};
}

EvalResult Evaluator::evalIf(const ExprVec &elements) {
  auto argCount = elements.size() - 1;
  if (argCount != 2 && argCount != 3) {
    throw std::runtime_error("expected two or three arguments, got: " +
                             std::to_string(argCount));
  }

  auto evaluatedCond = eval(elements[1]);
  if (const auto *cond = std::get_if<Bool>(&evaluatedCond)) {
    if (*cond == Bool::FALSE) {
      return argCount == 3 ? EvalResult{false, nullptr, elements[3]}
                           : EvalResult{true, nullptr, Nil::NIL};
    }
  }
  if (const auto *cond = std::get_if<Nil>(&evaluatedCond)) {
    return argCount == 3 ? EvalResult{false, nullptr, elements[3]}
                         : EvalResult{true, nullptr, Nil::NIL};
  }
  return EvalResult{false, nullptr, elements[2]};
}

Expr Evaluator::evalFnStar(const ExprVec &elements) {
  checkArgCount(elements.size() - 1, 2);

  if (const auto *params = std::get_if<Box<List>>(&elements[1])) {
    return doEvalFnStar((*params)->value, elements[2]);
  }
  if (const auto *params = std::get_if<Box<Vector>>(&elements[1])) {
    return doEvalFnStar((*params)->value, elements[2]);
  }

  throw std::runtime_error("function params are not a list: " +
                           prStr(elements[1], true));
}

Expr Evaluator::doEvalFnStar(const ExprVec &params, const Expr &body) {
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

EvalResult Evaluator::applyFunction(const ExprVec &calleeAndArgs) {
  const auto &callee = calleeAndArgs[0];
  if (const auto *funcPtr = std::get_if<Box<BuiltinFunction>>(&callee)) {
    const auto &func = *funcPtr;
    auto argCount = calleeAndArgs.size() - 1;
    if (func->arity != -1) {
      checkArgCount(argCount, func->arity);
    }
    const std::span<Expr> args(const_cast<ExprVec &>(calleeAndArgs));
    return EvalResult{true, nullptr, func->value(args.subspan(1))};
  }

  if (const auto *lambdaPtr = std::get_if<Box<Lambda>>(&callee)) {
    const auto &lambda = *lambdaPtr;
    auto paramCount = lambda->params.size();
    auto argCount = calleeAndArgs.size() - 1;
    if (lambda->variadic) {
      checkMinArgCount(argCount, paramCount - 1);
    } else {
      checkArgCount(argCount, paramCount);
    }

    auto lambdaEnv = std::make_shared<Env>(Env(lambda->env));
    ExprVec variadicArgs{lambdaEnv->get("list").getRaw()};
    for (int i = 0; i < argCount; i++) {
      const auto &arg = calleeAndArgs[i + 1];
      if (lambda->variadic) {
        if (i < paramCount - 1) {
          lambdaEnv->set(lambda->params[i], arg);
        } else {
          variadicArgs.push_back(arg);
        }
      } else {
        lambdaEnv->set(lambda->params[i], arg);
      }
    }
    if (lambda->variadic) {
      lambdaEnv->set(lambda->params[paramCount - 1], List{variadicArgs});
    }

    return EvalResult{false, lambdaEnv, lambda->body};
  }

  throw std::runtime_error("not a function: " + prStr(callee, true));
}
