#include "hasher.hpp"

size_t exprHash(const Expr &expr) {
  Hasher hasher;
  return std::visit(hasher, expr);
}
