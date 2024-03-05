#pragma once

#include "types.hpp"
#include <sstream>
#include <string>
#include <variant>
#include <vector>

class Printer {
private:
  bool printReadably;

public:
  explicit Printer(bool printReadably) : printReadably(printReadably) {}

  std::string operator()(const Nil & /*nil*/) const { return "nil"; }

  std::string operator()(const Int &num) const {
    return std::to_string(num.value);
  }

  std::string operator()(const Bool &boolean) const {
    switch (boolean) {
    case Bool::TRUE:
      return "true";
    case Bool::FALSE:
      return "false";
    }
  }

  std::string operator()(const String &str) const {
    return (printReadably ? '"' + escapeCharacters(str.value) + '"'
                          : str.value);
  }

  std::string operator()(const Symbol &sym) const { return sym.value; }

  std::string operator()(const Keyword &kwd) const { return ':' + kwd.value; }

  std::string operator()(const List &list) const {
    return prListLike(list.value, '(', ')');
  }

  std::string operator()(const Vector &vector) const {
    return prListLike(vector.value, '[', ']');
  }

  std::string operator()(const HashMap &hashmap) const {
    return prListLike(
        flattenKeyValuePairs(std::vector<std::pair<ExprPtr, ExprPtr>>(
            hashmap.value.begin(), hashmap.value.end())),
        '{', '}');
  }

  std::string operator()(const Quote &expr) const {
    return "(quote " + prExprPtr(expr.value) + ")";
  }

  std::string operator()(const QuasiQuote &expr) const {
    return "(quasiquote " + prExprPtr(expr.value) + ")";
  }

  std::string operator()(const Unquote &expr) const {
    return "(unquote " + prExprPtr(expr.value) + ")";
  }

  std::string operator()(const SpliceUnquote &expr) const {
    return "(splice-unquote " + prExprPtr(expr.value) + ")";
  }

  std::string operator()(const Deref &expr) const {
    return "(deref " + prExprPtr(expr.value) + ")";
  }

  std::string operator()(const MetaData &expr) const {
    return "(with-meta " + prExprPtr(expr.value) + " " + prExprPtr(expr.meta) +
           ")";
  }

  std::string operator()(const Atom &expr) const {
    return "(atom " + prExprPtr(expr.value) + ")";
  }

  std::string operator()(const BuiltinFunction &builtinFunc) const {
    return "#<" + builtinFunc.name + ">";
  }

  std::string operator()(const Lambda & /*lambda*/) const {
    return "#<function>";
  }

private:
  static std::string escapeCharacters(const std::string &input) {
    std::string output;
    output.reserve(input.length());

    for (const char chr : input) {
      switch (chr) {
      case '"':
        output += "\\\"";
        break;
      case '\\':
        output += "\\\\";
        break;
      case '\n':
        output += "\\n";
        break;
      default:
        output += chr;
      }
    }

    return output;
  }

  [[nodiscard]] std::string prListLike(const ExprPtrVec &elements,
                                       char startDelim, char endDelim) const {
    std::stringstream buffer;
    buffer << startDelim;
    for (const auto &elementPtr : elements) {
      buffer << prExprPtr(elementPtr) << " ";
    }

    if (elements.empty()) {
      buffer << endDelim;
      return buffer.str();
    }

    auto ret = buffer.str();
    ret[ret.size() - 1] = endDelim;
    return ret;
  }

  [[nodiscard]] std::string prExprPtr(const ExprPtr &exprPtr) const {
    return std::visit(*this, *exprPtr);
  }

  static ExprPtrVec
  flattenKeyValuePairs(const std::vector<std::pair<ExprPtr, ExprPtr>> &pairs) {
    ExprPtrVec vec;
    vec.reserve(pairs.size() * 2);
    for (const auto &pair : pairs) {
      vec.push_back(pair.first);
      vec.push_back(pair.second);
    }
    return vec;
  }
};

std::string prStr(const ExprPtr &exprPtr, bool printReadably);
