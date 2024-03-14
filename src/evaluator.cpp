#include "evaluator.hpp"
#include "constants.hpp"
#include "printer.hpp"
#include "types.hpp"
#include <iterator>
#include <unordered_set>
#include <variant>

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
  static const std::unordered_set<std::string> SPECIAL_FORM_SYMBOLS = {
      DEF_FORM_SYM,        DEFMACRO_FORM_SYM,
      LET_STAR_FORM_SYM,   DO_FORM_SYM,
      IF_FORM_SYM,         FN_STAR_FORM_SYM,
      QUOTE_FORM_SYM,      QUASIQUOTEEXPAND_FORM_SYM,
      QUASIQUOTE_FORM_SYM, MACROEXPAND_FORM_SYM,
  };

  const auto expandedExpr = macroexpand(expr);

  auto firstSym = firstSymbolOfList(expandedExpr);
  if (firstSym.has_value() && SPECIAL_FORM_SYMBOLS.contains(*firstSym)) {
    return evalSpecialForm(*firstSym, std::get<Box<List>>(expandedExpr)->value);
  }

  auto evaluatedExpr = std::visit(*this, expandedExpr);
  if (const auto *list = std::get_if<Box<List>>(&evaluatedExpr)) {
    auto &elements = (*list)->value;
    if (!elements.empty()) {
      auto callee = elements.front();
      elements.pop_front();
      return applyFunction(callee, elements);
    }
  }
  return EvalResult{true, nullptr, evaluatedExpr};
}

EvalResult Evaluator::evalSpecialForm(const std::string &specialSymbol,
                                      const ExprList &elements) {
  if (specialSymbol == DEF_FORM_SYM) {
    return EvalResult{true, nullptr, evalDefBang(elements)};
  }
  if (specialSymbol == DEFMACRO_FORM_SYM) {
    return EvalResult{true, nullptr, evalDefmacroBang(elements)};
  }
  if (specialSymbol == LET_STAR_FORM_SYM) {
    return evalLetStar(elements);
  }
  if (specialSymbol == DO_FORM_SYM) {
    return evalDo(elements);
  }
  if (specialSymbol == IF_FORM_SYM) {
    return evalIf(elements);
  }
  if (specialSymbol == FN_STAR_FORM_SYM) {
    return EvalResult{true, nullptr, evalFnStar(elements)};
  }
  if (specialSymbol == QUOTE_FORM_SYM) {
    return EvalResult{true, nullptr, *nth(elements, 1)};
  }
  if (specialSymbol == QUASIQUOTEEXPAND_FORM_SYM) {
    return EvalResult{true, nullptr, quasiquote(*nth(elements, 1))};
  }
  if (specialSymbol == QUASIQUOTE_FORM_SYM) {
    return EvalResult{false, nullptr, quasiquote(*nth(elements, 1))};
  }
  if (specialSymbol == MACROEXPAND_FORM_SYM) {
    checkArgCount(elements.size() - 1, 1);
    return EvalResult{true, nullptr, macroexpand(*nth(elements, 1))};
  }

  throw WrongTypeError("a special symbol", Symbol{specialSymbol});
}

Expr Evaluator::macroexpand(const Expr &expr) {
  auto curExpr = expr;
  auto func = findMacroFunc(curExpr);
  while (func.has_value() && func->getRaw().macro) {
    auto &list = std::get<Box<List>>(curExpr)->value;
    list.pop_front();
    curExpr = apply(func->getRaw(), list);
    func = findMacroFunc(curExpr);
  }

  return curExpr;
}

Expr Evaluator::evalDefBang(const ExprList &elements) {
  checkArgCount(elements.size() - 1, 2);
  return std::get<1>(defineBinding(
      env, *nth(elements, 1), [&]() { return this->eval(elements.back()); }));
}

Expr Evaluator::evalDefmacroBang(const ExprList &elements) {
  checkArgCount(elements.size() - 1, 2);
  const auto &bindingValueExpr = elements.back();
  auto firstSym = firstSymbolOfList(bindingValueExpr);
  if (!firstSym.has_value() || firstSym.value() != FN_STAR_FORM_SYM) {
    throw WrongTypeError("fn* form", bindingValueExpr);
  }

  return std::get<1>(defineBinding(env, *nth(elements, 1), [&]() {
    auto bindingValue = this->eval(bindingValueExpr);
    const auto &lambda = std::get<Box<Lambda>>(bindingValue);
    lambda->macro = true;
    return std::move(bindingValue);
  }));
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
  throw WrongTypeError("a list or vector", bindingsList);
}

template <typename Container>
EvalResult Evaluator::doEvalLetStar(const Container &bindingList,
                                    const Expr &letBody) {
  if (bindingList.empty()) {
    return EvalResult{false, env, letBody};
  }

  auto bindingListSize = bindingList.size();
  if (bindingListSize % 2 != 0) {
    throw MALError("odd number of elements in let* binding list");
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
    auto nameValue = defineBinding(letEnvPtr, bindingName,
                                   [&]() { return evaluator->eval(*iter); });
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
    throw WrongArgCountError("two or three", argCount);
  }

  auto evaluatedCond = eval(*nth(elements, 1));
  if (const auto *cond = std::get_if<Bool>(&evaluatedCond)) {
    if (*cond == Bool::FALSE) {
      return EvalResult{argCount == 2, nullptr,
                        argCount == 3 ? elements.back() : Nil::NIL};
    }
  }
  if (std::holds_alternative<Nil>(evaluatedCond)) {
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

  throw WrongTypeError("a list or vector", *mParams);
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
        throw MALError("more than one variadic function params specified");
      }

      paramNames.push_back(paramName->value);
      if (variadic) {
        variadicParamsCount++;
      }
      continue;
    }

    throw WrongTypeError("a symbol", param);
  }

  checkDupParamNames(paramNames);
  return Lambda{.params = std::move(paramNames),
                .variadic = variadic,
                .macro = false,
                .body = body,
                .env = std::make_shared<Env>(Env(env))};
}

Expr Evaluator::quasiquote(const Expr &expr) {
  if (const auto *list = std::get_if<Box<List>>(&expr)) {
    return quasiquoteList(*list);
  }

  if (const auto *vec = std::get_if<Box<Vector>>(&expr)) {
    ExprList elements{(*vec)->value.begin(), (*vec)->value.end()};
    if (const auto *sym = std::get_if<Symbol>(&((*vec)->value.front()))) {
      if (sym->value == UNQUOTE_FORM_SYM) {
        elements.front() = List{ExprList{
            Symbol{UNQUOTE_FORM_SYM},
            List{ExprList{Symbol{QUOTE_FORM_SYM}, Symbol{UNQUOTE_FORM_SYM}}}}};
      }
    }
    return List{
        ExprList{Symbol{VEC_FN_SYM}, quasiquote(std::move(List{elements}))}};
  }

  if (std::holds_alternative<Box<HashMap>>(expr) ||
      std::holds_alternative<Symbol>(expr)) {
    return List{ExprList{{Symbol{QUOTE_FORM_SYM}, expr}}};
  }

  return expr;
}

Expr Evaluator::quasiquoteList(const List &list) {
  const auto &elements = list.value;
  if (elements.empty()) {
    return list;
  }

  if (const auto *sym = std::get_if<Symbol>(&elements.front())) {
    if (sym->value == UNQUOTE_FORM_SYM && elements.size() > 1) {
      checkArgCount(elements.size(), 2);
      return elements.back();
    }
  }

  if (const auto *elt = std::get_if<Box<List>>(&elements.front())) {
    const auto &eltElements = (*elt)->value;
    if (!eltElements.empty()) {
      if (const auto *sym = std::get_if<Symbol>(&eltElements.front())) {
        if (sym->value == SPLICE_UNQUOTE_FORM_SYM) {
          checkArgCount(eltElements.size(), 2);
          return List{ExprList{{
              Symbol{CONCAT_FN_SYM},
              eltElements.back(),
              quasiquote(List{{std::next(elements.begin()), elements.end()}}),
          }}};
        }
      }
    }
  }

  return List{ExprList{
      Symbol{CONS_FN_SYM},
      quasiquote(elements.front()),
      quasiquote(List{{std::next(elements.begin()), elements.end()}}),
  }};
}

Expr Evaluator::apply(const Expr &callee, ExprList &args) {
  const auto &result = applyFunction(callee, args);
  if (result.evaluated) {
    return result.result;
  }
  Evaluator evaluator(result.env);
  return evaluator.eval(result.result);
}

EvalResult Evaluator::applyFunction(const Expr &callee, ExprList &args) {
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
    ExprList variadicArgs{};

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

  throw WrongTypeError("a function", callee);
}
