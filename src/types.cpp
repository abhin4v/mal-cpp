#include "types.hpp"

std::shared_ptr<Expr> makeBuiltinFunction(std::string name,
                                          BinaryFunction func) {
  return std::make_shared<Expr>(
      BuiltinFunction{std::move(name), std::move(func)});
}
