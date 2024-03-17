#include "types.hpp"

#include <immer/algorithm.hpp>

#include "equator.hpp"
#include "hasher.hpp"
#include "printer.hpp"

bool ExprEquals::operator()(const Expr &key1, const Expr &key2) const {
  return exprEquals(key1, key2);
}

size_t ExprHash::operator()(const Expr &key) const { return exprHash(key); }

HashMap makeHashMap(const ExprList &elements) {
  auto elementCount = elements.size();
  if (elementCount % 2 != 0) {
    throw MALError("odd number of elements in hash-map");
  }

  ExprMap map;

  for (auto iter = elements.begin(); iter != elements.end(); ++iter) {
    const auto &key = *iter;
    std::advance(iter, 1);
    const auto &val = *iter;
    map             = std::move(map).set(key, val);
  }

  return HashMap{map};
}

Expr makeBuiltinFunction(std::string name, int arity, ExprFunction func) {
  return BuiltinFunction{std::move(name), arity, std::move(func)};
}

template <>
Expr makeBuiltinFunction<bool>(const std::string &name, UnaryExprFunction<bool> func) {
  return makeBuiltinFunction(name, 1, [func = std::move(func)](const ExprList &args) {
    checkArgCount(args.size(), 1);
    return func(args.front()) ? Bool::TRUE : Bool::FALSE;
  });
}

template <>
Expr makeBuiltinFunction<Expr>(const std::string &name, UnaryExprFunction<Expr> func) {
  return makeBuiltinFunction(name, 1, [func = std::move(func)](const ExprList &args) {
    checkArgCount(args.size(), 1);
    return func(args.front());
  });
}

namespace {
template <typename T>
T applyBinaryFunction(const BinaryExprFunction<T> &func, const ExprList &args) {
  checkArgCount(args.size(), 2);
  return func(args.front(), args.back());
}

template <typename T>
T applyBinaryIntFunction(const BinaryIntFunction<T> &func, const ExprList &args) {
  return applyBinaryFunction<T>(
      [func = std::move(func)](const Expr &arg1, const Expr &arg2) {
        return func(intVal(arg1), intVal(arg2));
      },
      args);
}
}  // namespace

template <>
Expr makeBuiltinFunction<std::int64_t>(const std::string &name,
                                       BinaryIntFunction<std::int64_t> func) {
  return makeBuiltinFunction(name, 2, [func = std::move(func)](const ExprList &args) {
    return Int{applyBinaryIntFunction<std::int64_t>(func, args)};
  });
}

template <>
Expr makeBuiltinFunction<bool>(const std::string &name, BinaryIntFunction<bool> func) {
  return makeBuiltinFunction(name, 2, [func = std::move(func)](const ExprList &args) {
    return applyBinaryIntFunction<bool>(func, args) ? Bool::TRUE : Bool::FALSE;
  });
}

template <>
Expr makeBuiltinFunction<bool>(const std::string &name, BinaryExprFunction<bool> func) {
  return makeBuiltinFunction(name, 2, [func = std::move(func)](const ExprList &args) {
    return applyBinaryFunction<bool>(func, args) ? Bool::TRUE : Bool::FALSE;
  });
}

template <>
Expr makeBuiltinFunction<Expr>(const std::string &name, BinaryExprFunction<Expr> func) {
  return makeBuiltinFunction(name, 2, [func = std::move(func)](const ExprList &args) {
    return applyBinaryFunction<Expr>(func, args);
  });
}

Expr makeBuiltinFunction(const std::string &name, VariadicIntFunction<std::int64_t> func) {
  auto exprFunc = [func = std::move(func)](const ExprList &args) {
    return Int{
        func(immer::accumulate(args, immer::vector<std::int64_t>{}, [](auto acc, const auto &arg) {
          return acc.push_back(intVal(arg));
        }))};
  };

  return makeBuiltinFunction(name, -1, exprFunc);
}

void checkArgCount(size_t argCount, size_t expected) {
  if (argCount != expected) {
    throw WrongArgCountError(std::to_string(expected), argCount);
  }
}

void checkMinArgCount(size_t argCount, size_t expected) {
  if (argCount < expected) {
    throw WrongArgCountError("at least " + std::to_string(expected), argCount);
  }
}

std::int64_t intVal(const Expr &expr) {
  if (const auto *intExpr = std::get_if<Int>(&expr)) {
    return intExpr->value;
  }
  throw WrongTypeError("an integer", expr);
}

WrongTypeError::WrongTypeError(const std::string &expected, const Expr &actual)
    : MALError("wrong type: expected " + expected + ", got: " + prStr(actual, true)) {}
