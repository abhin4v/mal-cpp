#pragma once

#include "tokenizer.hpp"
#include "types.hpp"
#include <optional>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <vector>

class Reader {
private:
  std::vector<Token> tokens;
  int position{0};

public:
  explicit Reader(const std::vector<Token> &tokens) : tokens(tokens) {}

  bool eof() { return !peek().has_value(); }

  ExprPtr readForm();

private:
  std::vector<ExprPtr> readList(const Token &startDelimToken,
                                TokenType endDelimTokenType);

  ExprPtr readAtom();

  static std::int64_t parseUnsignedInteger(const std::string &lexeme) {
    std::int64_t num{0};
    for (auto chr : lexeme) {
      if (std::isdigit(chr) != 0) {
        num = num * DECIMAL_BASE + (chr - '0');
      } else {
        std::stringstream error;
        error << "not a digit: " << chr;
        throw std::runtime_error(error.str());
      }
    }
    return num;
  }

  std::optional<const Token> peek(int offset = 0) {
    if (position + offset >= tokens.size()) {
      return std::nullopt;
    }
    return std::make_optional(tokens[position + offset]);
  }

  Token advance() {
    auto token = peek();
    if (token.has_value()) {
      position++;
      return token.value();
    }
    throw std::runtime_error("at EOF");
  }

  void expect(const Token &expected) {
    auto const token = advance();
    if (token != expected) {
      std::stringstream error;
      error << "expected " << expected.toString() << ", got "
            << token.toString();
      throw std::runtime_error(error.str());
    }
  }

  static const int DECIMAL_BASE = 10;
};

ExprPtr readStr(const std::string &input);
