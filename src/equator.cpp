#include "equator.hpp"

bool exprEquals(const Expr &expr1, const Expr &expr2) {
  const Equator equator;
  return std::visit(equator, expr1, expr2);
}
