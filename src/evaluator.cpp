#include "evaluator.hpp"
#include "printer.hpp"
#include "types.hpp"

namespace {
std::int64_t intVal(const ExprPtr &exprPtr) {
  if (auto *expr = std::get_if<Int>(&*exprPtr)) {
    return expr->value;
  }
  throw std::runtime_error("not an integer: " + prStr(exprPtr, true));
}

ExprPtr callFunction(const ExprPtrVec &calleeAndArgs) {
  const auto &callee = calleeAndArgs[0];
  if (auto *func = std::get_if<BuiltinFunction>(&*callee)) {
    auto argCount = calleeAndArgs.size() - 1;
    if (argCount != 2) {
      throw std::runtime_error("expected two arguments, got: " +
                               std::to_string(argCount));
    }

    return std::make_shared<Expr>(
        Int{func->value(intVal(calleeAndArgs[1]), intVal(calleeAndArgs[2]))});
  }

  throw std::runtime_error("not a function: " + prStr(callee, true));
}
} // namespace

ExprPtr eval(const Env &env, const ExprPtr &exprPtr) {
  auto evaluatedExpr = std::visit(Evaluator(env), *exprPtr);

  if (auto *list = std::get_if<List>(&evaluatedExpr)) {
    if (!list->value.empty()) {
      return callFunction(list->value);
    }
  }
  return std::make_shared<Expr>(std::move(evaluatedExpr));
}
