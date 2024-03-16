#pragma once

#include "types.hpp"
#include <sstream>
#include <string>
#include <utility>
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
        flattenKeyValuePairs({hashmap.value.begin(), hashmap.value.end()}), '{',
        '}');
  }

  std::string operator()(const Atom &expr) const {
    return "(atom " + prExpr(expr.value) + ")";
  }

  std::string operator()(const BuiltinFunction &builtinFunc) const {
    return "#<" + builtinFunc.name + "/" + std::to_string(builtinFunc.arity) +
           ">";
  }

  std::string operator()(const Lambda &lambda) const {
    auto paramCount = lambda.params.size();
    return std::string(lambda.macro ? "%" : "#") + "<lambda/" +
           (lambda.variadic ? std::to_string(paramCount - 1) + "+"
                            : std::to_string(paramCount)) +
           ">";
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

  template <typename Container>
  [[nodiscard]] std::string prListLike(const Container &elements,
                                       char startDelim, char endDelim) const {
    std::stringstream buffer;
    buffer << startDelim;
    for (const auto &element : elements) {
      buffer << prExpr(element) << " ";
    }

    if (elements.empty()) {
      buffer << endDelim;
      return buffer.str();
    }

    auto ret = buffer.str();
    ret[ret.size() - 1] = endDelim;
    return ret;
  }

  [[nodiscard]] std::string prExpr(const Expr &expr) const {
    return std::visit(*this, expr);
  }

  static ExprList
  flattenKeyValuePairs(const std::vector<std::pair<Expr, Expr>> &pairs) {
    ExprList list;
    for (const auto &pair : pairs) {
      list = std::move(list).push_back(pair.first).push_back(pair.second);
    }
    return list;
  }
};

std::string prStr(const Expr &expr, bool printReadably);
