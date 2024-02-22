#include "printer.hpp"

std::string prStr(const std::shared_ptr<Expr> &expr, bool printReadably) {
  return std::visit(Printer(printReadably), *expr);
}
