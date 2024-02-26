#include "evaluator.hpp"
#include "types.hpp"

ExprPtr Evaluator::eval(const ExprPtr &exprPtr) {
  static const std::unordered_set<std::string> SPECIAL_SYMBOLS = {
      "def!",
      "let*",
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
