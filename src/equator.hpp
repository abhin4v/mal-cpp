#pragma once

#include "types.hpp"
#include <iterator>

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
  bool operator()(const Box<BuiltinFunction> &func1,
                  const Box<BuiltinFunction> &func2) const {
    return func1->name == func2->name && func1->arity == func2->arity;
  }
  bool operator()(const Lambda & /*func1*/, const Lambda & /*func2*/) const {
    return false;
  }
  bool operator()(const Box<Quote> &expr1, const Box<Quote> &expr2) const {
    return equateExpr(expr1->value, expr2->value);
  }
  bool operator()(const Box<QuasiQuote> &expr1,
                  const Box<QuasiQuote> &expr2) const {
    return equateExpr(expr1->value, expr2->value);
  }
  bool operator()(const Box<Unquote> &expr1, const Box<Unquote> &expr2) const {
    return equateExpr(expr1->value, expr2->value);
  }
  bool operator()(const Box<SpliceUnquote> &expr1,
                  const Box<SpliceUnquote> &expr2) const {
    return equateExpr(expr1->value, expr2->value);
  }
  bool operator()(const Box<Deref> &expr1, const Box<Deref> &expr2) const {
    return equateExpr(expr1->value, expr2->value);
  }
  bool operator()(const Box<MetaData> &expr1,
                  const Box<MetaData> &expr2) const {
    return equateExpr(expr1->value, expr2->value);
  }
  bool operator()(const Box<List> &list1, const Box<List> &list2) const {
    return equateElements(list1->value, list2->value);
  }
  bool operator()(const Box<List> &list, const Box<Vector> &vector) const {
    return equateElements(list->value, vector->value);
  }
  bool operator()(const Box<Vector> &vector1,
                  const Box<Vector> &vector2) const {
    return equateElements(vector1->value, vector2->value);
  }
  bool operator()(const Box<Vector> &vector, const Box<List> &list) const {
    return equateElements(vector->value, list->value);
  }
  bool operator()(const Box<HashMap> &hm1, const Box<HashMap> &hm2) const {
    if (hm1->value.size() != hm2->value.size()) {
      return false;
    }

    for (const auto &[key, value] : hm1->value) {
      if (!hm2->value.contains(key)) {
        return false;
      }

      if (!equateExpr(value, hm2->value.at(key))) {
        return false;
      }
    }
    return true;
  }

  bool operator()(const auto & /*expr1*/, const auto & /*expr2*/) const {
    return false;
  }

private:
  [[nodiscard]] bool equateExpr(const Expr &expr1, const Expr &expr2) const {
    return std::visit(*this, expr1, expr2);
  }

  template <typename Container1, typename Container2>
  [[nodiscard]] bool equateElements(const Container1 &elements,
                                    const Container2 &otherElements) const {
    if (elements.size() != otherElements.size()) {
      return false;
    }
    return std::equal(
        elements.begin(), elements.end(), otherElements.begin(),
        [&](auto expr1, auto expr2) { return equateExpr(expr1, expr2); });
  }
};

bool exprEquals(const Expr &expr1, const Expr &expr2);
