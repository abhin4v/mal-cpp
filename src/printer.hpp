#pragma once

#include "types.hpp"
#include <sstream>
#include <string>
#include <variant>

class Printer {
private:
  bool printReadably;

public:
  explicit Printer(bool printReadably) : printReadably(printReadably) {}

  std::string operator()(const String &str) const {
    return '"' + (printReadably ? escapeCharacters(str.value) : str.value) +
           '"';
  }
  std::string operator()(const Int &num) const {
    return std::to_string(num.value);
  }
  std::string operator()(const Symbol &str) const { return str.value; }
  std::string operator()(const List &list) const {
    std::stringstream buffer;
    buffer << "(";
    for (const auto &elementPtr : list.value) {
      auto *element = elementPtr.get();
      buffer << std::visit(*this, *element) << " ";
    }

    if (list.value.empty()) {
      buffer << ")";
      return buffer.str();
    }

    auto ret = buffer.str();
    ret[ret.size() - 1] = ')';
    return ret;
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
};

std::string prStr(const std::shared_ptr<Expr> &expr, bool printReadably);
