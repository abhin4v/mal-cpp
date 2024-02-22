#include "printer.hpp"

std::string prStr(const ExprPtr &expr, bool printReadably) {
  return std::visit(Printer(printReadably), *expr);
}
