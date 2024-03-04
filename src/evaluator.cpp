#include "evaluator.hpp"
#include "printer.hpp"
#include "types.hpp"

ExprPtr Evaluator::trampoliningEval(const ExprPtr &exprPtr) {
  auto exprP = exprPtr;
  while (true) {
    auto result = doEval(exprP);
    if (result.evaluated) {
      return result.result;
    }

    if (result.env != nullptr) {
      this->env = result.env;
    }

    exprP = result.result;
  }
}

ExprPtr Evaluator::eval(const ExprPtr &exprPtr) {
  auto result = doEval(exprPtr);
  if (result.evaluated) {
    return result.result;
  }

  if (result.env == nullptr) {
    return eval(result.result);
  }

  Evaluator evaluator(result.env);
  return evaluator.eval(result.result);
}

EvalResult Evaluator::doEval(const ExprPtr &exprPtr) {
  static const std::unordered_set<std::string> SPECIAL_SYMBOLS = {
      "def!", "let*", "do", "if", "fn*",
  };

  auto firstSym = firstSymbolOfList(exprPtr);
  if (firstSym.has_value() && SPECIAL_SYMBOLS.contains(*firstSym)) {
    return evalSpecialForm(*firstSym, std::get<List>(*exprPtr).value);
  }

  auto evaluatedExprPtr = std::visit(*this, *exprPtr);
  if (const auto *list = std::get_if<List>(&*evaluatedExprPtr)) {
    if (!list->value.empty()) {
      return applyFunction(list->value);
    }
  }
  return EvalResult{true, nullptr, evaluatedExprPtr};
}

EvalResult Evaluator::evalSpecialForm(const std::string &specialSymbol,
                                      const ExprPtrVec &elements) {
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

ExprPtr Evaluator::evalDefBang(const ExprPtrVec &elements) {
  checkArgCount(elements.size() - 1, 2);
  return std::get<1>(defineBinding(*this, env, elements[1], elements[2]));
}

EvalResult Evaluator::evalLetStar(const ExprPtrVec &elements) {
  checkArgCount(elements.size() - 1, 2);

  const auto &bindingsListPtr = elements[1];
  if (const auto *bindingList = std::get_if<List>(&*bindingsListPtr)) {
    return doEvalLetStar(bindingList->value, elements[2]);
  }
  if (const auto *bindingList = std::get_if<Vector>(&*bindingsListPtr)) {
    return doEvalLetStar(bindingList->value, elements[2]);
  }
  throw std::runtime_error("not a list or vector: " +
                           prStr(bindingsListPtr, true));
}

EvalResult Evaluator::doEvalLetStar(const ExprPtrVec &bindingList,
                                    const ExprPtr &letBodyPtr) {
  if (bindingList.empty()) {
    return EvalResult{false, env, letBodyPtr};
  }

  auto bindingListSize = bindingList.size();
  if (bindingListSize % 2 != 0) {
    throw std::runtime_error("odd number of elements in let* binding list");
  }

  Env letEnv(this->env);
  auto letEnvPtr = std::make_shared<Env>(letEnv);
  Evaluator letEvaluator(letEnvPtr);
  Evaluator *evaluator = this;

  std::unordered_map<std::string, ExprPtr> bindings;
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
    if (const auto *lambda = std::get_if<Lambda>(&*value)) {
      for (const auto &[name, value] : bindings) {
        lambda->env->set(name, value);
      }
    }
  }

  return EvalResult{false, evaluator->env, letBodyPtr};
}

EvalResult Evaluator::evalDo(const ExprPtrVec &elements) {
  if (elements.size() == 1) {
    return EvalResult{true, env, nilPtr()};
  }

  for (auto i = 1; i < elements.size() - 1; i++) {
    eval(elements[i]);
  }

  return EvalResult{false, nullptr, elements.back()};
}

EvalResult Evaluator::evalIf(const ExprPtrVec &elements) {
  auto argCount = elements.size() - 1;
  if (argCount != 2 && argCount != 3) {
    throw std::runtime_error("expected two or three arguments, got: " +
                             std::to_string(argCount));
  }

  auto evaluatedCond = eval(elements[1]);
  if (const auto *cond = std::get_if<Bool>(&*evaluatedCond)) {
    if (*cond == Bool::FALSE) {
      return argCount == 3 ? EvalResult{false, nullptr, elements[3]}
                           : EvalResult{true, nullptr, nilPtr()};
    }
  }
  if (const auto *cond = std::get_if<Nil>(&*evaluatedCond)) {
    return argCount == 3 ? EvalResult{false, nullptr, elements[3]}
                         : EvalResult{true, nullptr, nilPtr()};
  }
  return EvalResult{false, nullptr, elements[2]};
}

ExprPtr Evaluator::evalFnStar(const ExprPtrVec &elements) {
  checkArgCount(elements.size() - 1, 2);

  if (const auto *params = std::get_if<List>(&*elements[1])) {
    return doEvalFnStar(params->value, elements[2]);
  }
  if (const auto *params = std::get_if<Vector>(&*elements[1])) {
    return doEvalFnStar(params->value, elements[2]);
  }

  throw std::runtime_error("function params are not a list: " +
                           prStr(elements[1], true));
}

ExprPtr Evaluator::doEvalFnStar(const ExprPtrVec &params, const ExprPtr &body) {
  ParamNames paramNames;
  paramNames.reserve(params.size());
  bool variadic{false};
  int variadicParamsCount{0};
  for (const auto &param : params) {
    if (const auto *paramName = std::get_if<Symbol>(&*param)) {
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
  return std::make_shared<Expr>(Lambda{.params = std::move(paramNames),
                                       .variadic = variadic,
                                       .body = body,
                                       .env = std::make_shared<Env>(Env(env))});
}

EvalResult Evaluator::applyFunction(const ExprPtrVec &calleeAndArgs) {
  const auto &callee = calleeAndArgs[0];
  if (const auto *func = std::get_if<BuiltinFunction>(&*callee)) {
    auto argCount = calleeAndArgs.size() - 1;
    if (func->arity != -1) {
      checkArgCount(argCount, func->arity);
    }
    const std::span<ExprPtr> args(const_cast<ExprPtrVec &>(calleeAndArgs));
    return EvalResult{true, nullptr, func->value(args.subspan(1))};
  }

  if (const auto *lambda = std::get_if<Lambda>(&*callee)) {
    auto paramCount = lambda->params.size();
    auto argCount = calleeAndArgs.size() - 1;
    if (lambda->variadic) {
      checkMinArgCount(argCount, paramCount - 1);
    } else {
      checkArgCount(argCount, paramCount);
    }

    auto lambdaEnv = std::make_shared<Env>(Env(lambda->env));
    ExprPtrVec variadicArgs{lambdaEnv->get("list")};
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
      auto arg = std::make_shared<Expr>(List{variadicArgs});
      lambdaEnv->set(lambda->params[paramCount - 1], arg);
    }

    return EvalResult{false, lambdaEnv, lambda->body};
  }

  throw std::runtime_error("not a function: " + prStr(callee, true));
}
