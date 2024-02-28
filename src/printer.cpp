#include "printer.hpp"

std::string prStr(const ExprPtr &exprPtr, bool printReadably) {
  return std::visit(Printer(printReadably), *exprPtr);
}
