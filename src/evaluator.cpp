#include "evaluator.hpp"
#include "printer.hpp"
#include "types.hpp"

ExprPtr Evaluator::eval(const ExprPtr &exprPtr) {
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
      return callFunction(list->value);
    }
  }
  return evaluatedExprPtr;
}

ExprPtr Evaluator::evalSpecialForm(const std::string &specialSymbol,
                                   const ExprPtrVec &elements) {
  if (specialSymbol == "def!") {
    return evalDefBang(elements);
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
    return evalFnStar(elements);
  }

  throw std::runtime_error("not a special symbol: " + specialSymbol);
}

ExprPtr Evaluator::evalDefBang(const ExprPtrVec &elements) {
  checkParamCount(elements.size() - 1, 2);
  return defineBinding(*this, env, elements[1], elements[2]);
}

ExprPtr Evaluator::evalLetStar(const ExprPtrVec &elements) {
  checkParamCount(elements.size() - 1, 2);

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

ExprPtr Evaluator::doEvalLetStar(const ExprPtrVec &bindingList,
                                 const ExprPtr &letBodyPtr) {
  if (bindingList.empty()) {
    return this->eval(letBodyPtr);
  }

  auto bindingListSize = bindingList.size();
  if (bindingListSize % 2 != 0) {
    throw std::runtime_error("odd number of elements in let* binding list");
  }

  Env letEnv(this->env);
  Evaluator *evaluator = this;

  for (auto i = 0; i < bindingListSize; i += 2) {
    auto letEnvPtr = std::make_shared<Env>(letEnv);
    defineBinding(*evaluator, letEnvPtr, bindingList[i], bindingList[i + 1]);
    Evaluator letEvaluator(letEnvPtr);
    evaluator = &letEvaluator;
    letEnv = Env(letEnvPtr);
  }

  return evaluator->eval(letBodyPtr);
}

ExprPtr Evaluator::evalDo(const ExprPtrVec &elements) {
  ExprPtr ret = nilPtr();

  for (auto i = 1; i < elements.size(); i++) {
    ret = eval(elements[i]);
  }

  return ret;
}

ExprPtr Evaluator::evalIf(const ExprPtrVec &elements) {
  auto argCount = elements.size() - 1;
  if (argCount != 2 && argCount != 3) {
    throw std::runtime_error("expected two or three arguments, got: " +
                             std::to_string(argCount));
  }

  auto evaluatedCond = eval(elements[1]);
  if (const auto *cond = std::get_if<Bool>(&*evaluatedCond)) {
    if (*cond == Bool::FALSE) {
      return argCount == 3 ? eval(elements[3]) : nilPtr();
    }
  }
  if (const auto *cond = std::get_if<Nil>(&*evaluatedCond)) {
    return argCount == 3 ? eval(elements[3]) : nilPtr();
  }
  return eval(elements[2]);
}

ExprPtr Evaluator::evalFnStar(const ExprPtrVec &elements) {
  checkParamCount(elements.size() - 1, 2);

  if (const auto *params = std::get_if<List>(&*elements[1])) {
    std::vector<std::string> paramNames;
    paramNames.reserve(params->value.size());
    for (const auto &param : params->value) {
      if (const auto *paramName = std::get_if<Symbol>(&*param)) {
        paramNames.push_back(paramName->value);
        continue;
      }

      throw std::runtime_error("function param is not symbol: " +
                               prStr(param, true));
    }

    return std::make_shared<Expr>(
        Lambda{std::move(paramNames), elements[2], env});
  }

  throw std::runtime_error("function params are not a list: " +
                           prStr(elements[1], true));
}

ExprPtr Evaluator::callFunction(const ExprPtrVec &calleeAndArgs) {
  const auto &callee = calleeAndArgs[0];
  if (const auto *func = std::get_if<BuiltinFunction>(&*callee)) {
    checkMinParamCount(calleeAndArgs.size() - 1, 1);
    const std::span<ExprPtr> args(const_cast<ExprPtrVec &>(calleeAndArgs));
    return func->value(args.subspan(1));
  }

  if (const auto *lambda = std::get_if<Lambda>(&*callee)) {
    auto paramCount = lambda->params.size();
    checkParamCount(calleeAndArgs.size() - 1, paramCount);

    Env lambdaEnv(lambda->env);
    for (int i = 0; i < paramCount; i++) {
      lambdaEnv.set(lambda->params[i], eval(calleeAndArgs[i + 1]));
    }

    Evaluator lambdaEvaluator(std::make_shared<Env>(lambdaEnv));
    return lambdaEvaluator.eval(lambda->body);
  }

  throw std::runtime_error("not a function: " + prStr(callee, true));
}
