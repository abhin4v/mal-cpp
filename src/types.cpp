#include "types.hpp"
#include "equator.hpp"
#include "hasher.hpp"
#include "printer.hpp"

bool ExprPtrEquals::operator()(const ExprPtr &key1, const ExprPtr &key2) const {
  return exprEquals(*key1, *key2);
}

size_t ExprPtrHash::operator()(const ExprPtr &key) const {
  return exprHash(*key);
}

std::shared_ptr<Expr> makeBuiltinFunction(std::string name, ExprFunction func) {
  return std::make_shared<Expr>(
      BuiltinFunction{std::move(name), std::move(func)});
}

std::shared_ptr<Expr> makeBuiltinFunction(std::string name,
                                          BinaryIntFunction func) {
  auto exprFunc = [&](const ExprPtrSpan &args) {
    checkParamCount(args.size(), 2);
    return std::make_shared<Expr>(Int{func(intVal(args[0]), intVal(args[1]))});
  };

  return std::make_shared<Expr>(
      BuiltinFunction{std::move(name), std::move(exprFunc)});
}

std::shared_ptr<Expr> makeBuiltinFunction(std::string name,
                                          VariadicIntFunction func) {
  auto exprFunc = [&](const ExprPtrSpan &args) {
    std::vector<std::int64_t> intArgs;
    intArgs.reserve(args.size());
    std::transform(args.begin(), args.end(), std::back_inserter(intArgs),
                   [](auto arg) { return intVal(arg); });

    return std::make_shared<Expr>(Int{func(intArgs)});
  };

  return std::make_shared<Expr>(
      BuiltinFunction{std::move(name), std::move(exprFunc)});
}

ExprPtr nilPtr() {
  static const auto nilPtr = std::make_shared<const Expr>(Nil::NIL);
  return nilPtr;
}

ExprPtr truePtr() {
  static const auto truePtr = std::make_shared<const Expr>(Bool::TRUE);
  return truePtr;
}

ExprPtr falsePtr() {
  static const auto falsePtr = std::make_shared<const Expr>(Bool::FALSE);
  return falsePtr;
}

void checkParamCount(size_t paramCount, size_t expected) {
  if (paramCount != expected) {
    throw std::runtime_error("expected " + std::to_string(expected) +
                             " arguments, got: " + std::to_string(paramCount));
  }
}

void checkMinParamCount(size_t paramCount, size_t expected) {
  if (paramCount < expected) {
    throw std::runtime_error("expected at least " + std::to_string(expected) +
                             " arguments, got: " + std::to_string(paramCount));
  }
}

std::int64_t intVal(const ExprPtr &exprPtr) {
  if (const auto *expr = std::get_if<Int>(&*exprPtr)) {
    return expr->value;
  }
  throw std::runtime_error("not an integer: " + prStr(exprPtr, true));
}
