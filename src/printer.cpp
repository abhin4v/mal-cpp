#include "printer.hpp"

std::string prStr(const Expr &expr, bool printReadably) {
  return std::visit(Printer(printReadably), expr);
}
