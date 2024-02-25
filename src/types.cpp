#include "types.hpp"

std::shared_ptr<Expr> makeBuiltinFunction(const std::string &name,
                                          BinaryFunction func) {
  return std::make_shared<Expr>(BuiltinFunction{name, std::move(func)});
}
