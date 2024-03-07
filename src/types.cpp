#include "types.hpp"
#include "equator.hpp"
#include "hasher.hpp"
#include "printer.hpp"

bool ExprEquals::operator()(const Expr &key1, const Expr &key2) const {
  return exprEquals(key1, key2);
}

size_t ExprHash::operator()(const Expr &key) const { return exprHash(key); }

Expr makeBuiltinFunction(std::string name, int arity, ExprFunction func) {
  return BuiltinFunction{std::move(name), arity, std::move(func)};
}

Expr makeBuiltinFunction(const std::string &name, UnaryFunction func) {
  auto exprFunc = [func = std::move(func)](const ExprSpan &args) {
    checkArgCount(args.size(), 1);
    return func(args[0]);
  };

  return makeBuiltinFunction(name, 1, exprFunc);
}

Expr makeBuiltinFunction(const std::string &name, UnaryBooleanFunction func) {
  auto exprFunc = [func = std::move(func)](const ExprSpan &args) {
    checkArgCount(args.size(), 1);
    return func(args[0]) ? Bool::TRUE : Bool::FALSE;
  };

  return makeBuiltinFunction(name, 1, exprFunc);
}

Expr makeBuiltinFunction(const std::string &name, BinaryIntFunction func) {
  auto exprFunc = [func = std::move(func)](const ExprSpan &args) {
    checkArgCount(args.size(), 2);
    return Int{func(intVal(args[0]), intVal(args[1]))};
  };

  return makeBuiltinFunction(name, 2, exprFunc);
}

Expr makeBuiltinFunction(const std::string &name, VariadicIntFunction func) {
  auto exprFunc = [func = std::move(func)](const ExprSpan &args) {
    std::vector<std::int64_t> intArgs;
    intArgs.reserve(args.size());
    std::transform(args.begin(), args.end(), std::back_inserter(intArgs),
                   [](auto arg) { return intVal(arg); });

    return Int{func(intArgs)};
  };

  return makeBuiltinFunction(name, -1, exprFunc);
}

void checkArgCount(size_t argCount, size_t expected) {
  if (argCount != expected) {
    throw std::runtime_error("expected " + std::to_string(expected) +
                             " arguments, got: " + std::to_string(argCount));
  }
}

void checkMinArgCount(size_t argCount, size_t expected) {
  if (argCount < expected) {
    throw std::runtime_error("expected at least " + std::to_string(expected) +
                             " arguments, got: " + std::to_string(argCount));
  }
}

std::int64_t intVal(const Expr &expr) {
  if (const auto *intExpr = std::get_if<Int>(&expr)) {
    return intExpr->value;
  }
  throw std::runtime_error("not an integer: " + prStr(expr, true));
}
