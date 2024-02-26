#include "types.hpp"

std::shared_ptr<Expr> makeBuiltinFunction(std::string name,
                                          BinaryFunction func) {
  return std::make_shared<Expr>(
      BuiltinFunction{std::move(name), std::move(func)});
}

ExprPtr nilPtr() {
  static const auto nilPtr = std::make_shared<const Expr>(Nil::NIL);
  return nilPtr;
}
