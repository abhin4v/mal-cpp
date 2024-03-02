#pragma once

#include "types.hpp"

class Equator {
public:
  bool operator()(const Nil & /*nil1*/, const Nil & /*nil2*/) const {
    return true;
  }
  bool operator()(const Int &num1, const Int &num2) const {
    return num1.value == num2.value;
  }
  bool operator()(const Bool &boolean1, const Bool &boolean2) const {
    return boolean1 == boolean2;
  }
  bool operator()(const String &str1, const String &str2) const {
    return str1.value == str2.value;
  }
  bool operator()(const Keyword &kwd1, const Keyword &kwd2) const {
    return kwd1.value == kwd2.value;
  }
  bool operator()(const Symbol &sym1, const Symbol &sym2) const {
    return sym1.value == sym2.value;
  }
  bool operator()(const BuiltinFunction &func1,
                  const BuiltinFunction &func2) const {
    return func1.name == func2.name && func1.arity == func2.arity;
  }
  bool operator()(const Lambda & /*func1*/, const Lambda & /*func2*/) const {
    return false;
  }
  bool operator()(const Quote &expr1, const Quote &expr2) const {
    return equateExprPtr(expr1.value, expr2.value);
  }
  bool operator()(const QuasiQuote &expr1, const QuasiQuote &expr2) const {
    return equateExprPtr(expr1.value, expr2.value);
  }
  bool operator()(const Unquote &expr1, const Unquote &expr2) const {
    return equateExprPtr(expr1.value, expr2.value);
  }
  bool operator()(const SpliceUnquote &expr1,
                  const SpliceUnquote &expr2) const {
    return equateExprPtr(expr1.value, expr2.value);
  }
  bool operator()(const Deref &expr1, const Deref &expr2) const {
    return equateExprPtr(expr1.value, expr2.value);
  }
  bool operator()(const MetaData &expr1, const MetaData &expr2) const {
    return equateExprPtr(expr1.value, expr2.value);
  }
  bool operator()(const List &list1, const List &list2) const {
    return equateElements(list1.value, list2.value);
  }
  bool operator()(const List &list, const Vector &vector) const {
    return equateElements(list.value, vector.value);
  }
  bool operator()(const Vector &vector1, const Vector &vector2) const {
    return equateElements(vector1.value, vector2.value);
  }
  bool operator()(const Vector &vector, const List &list) const {
    return equateElements(vector.value, list.value);
  }
  bool operator()(const HashMap &hm1, const HashMap &hm2) const {
    if (hm1.value.size() != hm2.value.size()) {
      return false;
    }

    for (const auto &[key, value] : hm1.value) {
      if (!hm2.value.contains(key)) {
        return false;
      }

      if (!equateExprPtr(value, hm2.value.at(key))) {
        return false;
      }
    }
    return true;
  }

  bool operator()(const auto & /*expr1*/, const auto & /*expr2*/) const {
    return false;
  }

private:
  [[nodiscard]] bool equateExprPtr(const ExprPtr &exprPtr1,
                                   const ExprPtr &exprPtr2) const {
    return std::visit(*this, *exprPtr1, *exprPtr2);
  }

  [[nodiscard]] bool equateElements(const ExprPtrVec &elements,
                                    const ExprPtrVec &otherElements) const {
    if (elements.size() != otherElements.size()) {
      return false;
    }
    for (auto i = 0; i < elements.size(); i++) {
      if (!equateExprPtr(elements[i], otherElements[i])) {
        return false;
      }
    }
    return true;
  }
};

bool exprEquals(const Expr &expr1, const Expr &expr2);
