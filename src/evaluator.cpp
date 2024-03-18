#include "evaluator.hpp"

#include <iterator>
#include <unordered_set>
#include <variant>

#include "constants.hpp"
#include "errors.hpp"
#include "printer.hpp"
#include "types.hpp"

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

EvalResult Evaluator::doEval(const Expr &expr) {
  static const std::unordered_set<std::string> SPECIAL_FORM_SYMBOLS = {
      DEF_FORM_SYM,        DEFMACRO_FORM_SYM,    LET_STAR_FORM_SYM, DO_FORM_SYM,
      IF_FORM_SYM,         FN_STAR_FORM_SYM,     QUOTE_FORM_SYM,    QUASIQUOTEEXPAND_FORM_SYM,
      QUASIQUOTE_FORM_SYM, MACROEXPAND_FORM_SYM, TRY_STAR_FORM_SYM,
  };

  const auto expandedExpr = macroexpand(expr);

  auto firstSym           = firstSymbolOfList(expandedExpr);
  if (firstSym.has_value() && SPECIAL_FORM_SYMBOLS.contains(*firstSym)) {
    return evalSpecialForm(*firstSym, std::get<Box<List>>(expandedExpr)->value.drop(1));
  }

  auto evaluatedExpr = std::visit(*this, expandedExpr);
  if (const auto *list = std::get_if<Box<List>>(&evaluatedExpr)) {
    auto &elements = (*list)->value;
    if (!elements.empty()) {
      return applyFunction(elements.front(), elements.drop(1));
    }
  }
  return EvalResult{true, nullptr, evaluatedExpr};
}

EvalResult Evaluator::evalSpecialForm(const std::string &specialSymbol, const ExprList &args) {
  if (specialSymbol == DEF_FORM_SYM) {
    return EvalResult{true, nullptr, evalDefBang(args)};
  }
  if (specialSymbol == DEFMACRO_FORM_SYM) {
    return EvalResult{true, nullptr, evalDefmacroBang(args)};
  }
  if (specialSymbol == LET_STAR_FORM_SYM) {
    return evalLetStar(args);
  }
  if (specialSymbol == DO_FORM_SYM) {
    return evalDo(args);
  }
  if (specialSymbol == IF_FORM_SYM) {
    return evalIf(args);
  }
  if (specialSymbol == FN_STAR_FORM_SYM) {
    return EvalResult{true, nullptr, evalFnStar(args)};
  }
  if (specialSymbol == QUOTE_FORM_SYM) {
    checkArgCount(args.size(), 1);
    return EvalResult{true, nullptr, args.front()};
  }
  if (specialSymbol == QUASIQUOTEEXPAND_FORM_SYM) {
    checkArgCount(args.size(), 1);
    return EvalResult{true, nullptr, quasiquote(args.front())};
  }
  if (specialSymbol == QUASIQUOTE_FORM_SYM) {
    checkArgCount(args.size(), 1);
    return EvalResult{false, nullptr, quasiquote(args.front())};
  }
  if (specialSymbol == MACROEXPAND_FORM_SYM) {
    checkArgCount(args.size(), 1);
    return EvalResult{true, nullptr, macroexpand(args.front())};
  }
  if (specialSymbol == TRY_STAR_FORM_SYM) {
    checkMinArgCount(args.size(), 1);
    return EvalResult{true, nullptr, evalTryStar(args)};
  }

  throw WrongTypeError("a special symbol", Symbol{specialSymbol});
}

Expr Evaluator::evalDefBang(const ExprList &args) {
  checkArgCount(args.size(), 2);
  return std::get<1>(defineBinding(env, args.front(), [&]() { return this->eval(args.back()); }));
}

Expr Evaluator::evalDefmacroBang(const ExprList &args) {
  checkArgCount(args.size(), 2);
  const auto &bindingValueExpr = args.back();

  return std::get<1>(defineBinding(env, args.front(), [&]() {
    auto bindingValue = this->eval(bindingValueExpr);

    if (const auto *lambda = std::get_if<Box<Lambda>>(&bindingValue)) {
      auto firstSym = firstSymbolOfList(bindingValueExpr);
      if (firstSym.has_value() && firstSym.value() == FN_STAR_FORM_SYM) {
        (*lambda)->macro = true;
        return bindingValue;
      }
      auto lambdaCopy  = lambda->getRaw();
      lambdaCopy.macro = true;
      return Expr{lambdaCopy};
    }

    throw WrongTypeError("a function", bindingValue);
  }));
}

EvalResult Evaluator::evalLetStar(const ExprList &args) {
  checkArgCount(args.size(), 2);

  const auto &bindingsList = args.front();
  if (const auto *bindingList = std::get_if<Box<List>>(&bindingsList)) {
    return doEvalLetStar((*bindingList)->value, args.back());
  }
  if (const auto *bindingList = std::get_if<Box<Vector>>(&bindingsList)) {
    return doEvalLetStar((*bindingList)->value, args.back());
  }
  throw WrongTypeError("a list or vector", bindingsList);
}

template <typename Container>
EvalResult Evaluator::doEvalLetStar(const Container &bindingList, const Expr &letBody) {
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
    auto nameValue =
        defineBinding(letEnvPtr, bindingName, [&]() { return evaluator->eval(*iter); });
    bindings[nameValue.first] = nameValue.second;
    evaluator                 = &letEvaluator;
    letEnv                    = Env(letEnvPtr);
    letEnvPtr                 = std::make_shared<Env>(letEnv);
    letEvaluator              = Evaluator(letEnvPtr);
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

std::pair<std::string, Expr> Evaluator::defineBinding(
    const EnvPtr &env, const Expr &bindingNameExpr,
    const std::function<Expr()> &makeBindingValueExpr) {
  if (const auto *bindingName = std::get_if<Symbol>(&bindingNameExpr)) {
    auto evaluatedBindingValue = makeBindingValueExpr();
    auto name                  = bindingName->value;
    env->set(name, evaluatedBindingValue);
    return {name, evaluatedBindingValue};
  }
  throw WrongTypeError("a symbol", bindingNameExpr);
}

EvalResult Evaluator::evalDo(const ExprList &args) {
  const auto argCount = args.size();
  if (argCount == 0) {
    return EvalResult{true, env, Nil::NIL};
  }

  const auto doArgs = args.take(argCount - 1);
  // skip first and last arg
  for (const auto &arg : doArgs) {
    eval(arg);
  }

  return EvalResult{false, nullptr, args.back()};
}

EvalResult Evaluator::evalIf(const ExprList &args) {
  auto argCount = args.size();
  if (argCount != 2 && argCount != 3) {
    throw WrongArgCountError("two or three", argCount);
  }

  auto evaluatedCond = eval(args.front());
  if (const auto *cond = std::get_if<Bool>(&evaluatedCond)) {
    if (*cond == Bool::FALSE) {
      return EvalResult{argCount == 2, nullptr, argCount == 3 ? args.back() : Nil::NIL};
    }
  }
  if (std::holds_alternative<Nil>(evaluatedCond)) {
    return EvalResult{argCount == 2, nullptr, argCount == 3 ? args.back() : Nil::NIL};
  }
  return EvalResult{false, nullptr, args.at(1)};
}

Expr Evaluator::evalFnStar(const ExprList &args) {
  checkArgCount(args.size(), 2);

  const auto &mParams = args.front();
  if (const auto *params = std::get_if<Box<List>>(&mParams)) {
    return doEvalFnStar((*params).getRaw(), args.back());
  }
  if (const auto *params = std::get_if<Box<Vector>>(&mParams)) {
    return doEvalFnStar((*params).getRaw(), args.back());
  }

  throw WrongTypeError("a list or vector", mParams);
}

template <typename Container>
Expr Evaluator::doEvalFnStar(const Container &params, const Expr &body) {
  ParamNames paramNames;

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

      paramNames = std::move(paramNames).push_back(paramName->value);
      if (variadic) {
        variadicParamsCount++;
      }
      continue;
    }

    throw WrongTypeError("a symbol", param);
  }

  checkDupParamNames(paramNames);
  return Lambda{.params   = std::move(paramNames),
                .variadic = variadic,
                .macro    = false,
                .body     = body,
                .env      = std::make_shared<Env>(Env(env))};
}

Expr Evaluator::evalTryStar(const ExprList &args) {
  const Expr &expr = args.front();
  if (args.size() == 1) {
    return eval(expr);
  }

  checkArgCount(args.size(), 2);
  const auto &catchExpr = args.at(1);
  auto firstSym         = firstSymbolOfList(catchExpr);
  if (!firstSym.has_value() || firstSym.value() != CATCH_STAR_FORM_SYM) {
    throw MALError("try* must contain catch* form, got: " + Printer::prStr(catchExpr, true));
  }

  const auto &catchForm = std::get<Box<List>>(catchExpr)->value;
  checkArgCount(catchForm.size(), 3);

  const auto &catchVal = catchForm.at(1);
  if (const auto *catchSym = std::get_if<Symbol>(&catchVal)) {
    Lambda handler{
        .params   = {catchSym->value},
        .variadic = false,
        .macro    = false,
        .body     = catchForm.at(2),
        .env      = env,
    };
    try {
      return eval(expr);
    } catch (const MALException &e) {
      return apply(handler, {e.getValue()});
    } catch (const MALError &e) {
      return apply(handler, {String{e.what()}});
    }
  }
  throw WrongTypeError("a symbol", catchVal);
}

Expr Evaluator::macroexpand(const Expr &expr) {
  auto curExpr = expr;
  auto func    = findMacroFunc(curExpr);
  while (func.has_value()) {
    auto &list = std::get<Box<List>>(curExpr)->value;
    curExpr    = apply(func->getRaw(), list.drop(1));
    func       = findMacroFunc(curExpr);
  }

  return curExpr;
}

std::optional<Box<Lambda>> Evaluator::findMacroFunc(const Expr &expr) {
  auto firstSym = firstSymbolOfList(expr);
  if (!firstSym.has_value()) {
    return std::nullopt;
  }

  const auto &val = env->find(firstSym.value());
  if (!val.has_value()) {
    return std::nullopt;
  }

  if (const auto *lambda = std::get_if<Box<Lambda>>(&val.value().getRaw())) {
    return (*lambda)->macro ? std::make_optional(*lambda) : std::nullopt;
  }

  return std::nullopt;
}

Expr Evaluator::quasiquote(const Expr &expr) {
  if (const auto *list = std::get_if<Box<List>>(&expr)) {
    return quasiquoteList(*list);
  }

  if (const auto *vec = std::get_if<Box<Vector>>(&expr)) {
    ExprList elements = (*vec)->value;
    if (const auto *sym = std::get_if<Symbol>(&(elements.front()))) {
      if (sym->value == UNQUOTE_FORM_SYM) {
        elements = std::move(elements).set(
            0, List{{Symbol{UNQUOTE_FORM_SYM},
                     List{{Symbol{QUOTE_FORM_SYM}, Symbol{UNQUOTE_FORM_SYM}}}}});
      }
    }
    return List{{Symbol{VEC_FN_SYM}, quasiquote(std::move(List{elements}))}};
  }

  if (std::holds_alternative<Box<HashMap>>(expr) || std::holds_alternative<Symbol>(expr)) {
    return List{{{Symbol{QUOTE_FORM_SYM}, expr}}};
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
          return List{{{
              Symbol{CONCAT_FN_SYM},
              eltElements.back(),
              quasiquote(List{{std::next(elements.begin()), elements.end()}}),
          }}};
        }
      }
    }
  }

  return List{{
      Symbol{CONS_FN_SYM},
      quasiquote(elements.front()),
      quasiquote(List{{std::next(elements.begin()), elements.end()}}),
  }};
}

Expr Evaluator::apply(const Expr &callee, const ExprList &args) {
  const auto &result = applyFunction(callee, args);
  if (result.evaluated) {
    return result.result;
  }
  Evaluator evaluator(result.env);
  return evaluator.eval(result.result);
}

EvalResult Evaluator::applyFunction(const Expr &callee, const ExprList &args) {
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
    auto paramCount    = lambda->params.size();

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
          variadicArgs = std::move(variadicArgs).push_back(arg);
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
