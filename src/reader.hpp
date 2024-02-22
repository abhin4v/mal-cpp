#pragma once

#include "tokenizer.hpp"
#include "types.hpp"
#include <optional>
#include <sstream>
#include <stdexcept>
#include <vector>

class Reader {
private:
  std::vector<Token> tokens;
  int position{0};

public:
  explicit Reader(const std::vector<Token> &tokens)
      : tokens(removeComments(tokens)) {}

  bool eof() { return !peek().has_value(); }

  std::shared_ptr<Expr> readForm() {
    auto oToken = peek();

    if (oToken.has_value()) {
      auto token = oToken.value();
      switch (token.type) {
      case TokenType::LEFT_PAREN:
        return readList();
      default:
        return readAtom();
      }
    }

    throw std::runtime_error("no tokens");
  }

private:
  static std::vector<Token> removeComments(const std::vector<Token> &tokens) {
    std::vector<Token> tokensSansComments;
    tokensSansComments.reserve(tokens.size());

    std::copy_if(tokens.begin(), tokens.end(),
                 std::back_inserter(tokensSansComments),
                 [](auto token) { return token.type != TokenType::COMMENT; });

    return tokensSansComments;
  }

  std::shared_ptr<Expr> readList() {
    expect(Token{TokenType::LEFT_PAREN, "("});
    std::vector<std::shared_ptr<Expr>> elements;

    while (true) {
      auto oToken = peek();
      if (oToken.has_value()) {
        auto token = oToken.value();
        switch (token.type) {
        case TokenType::RIGHT_PAREN:
          advance();
          return std::make_shared<Expr>(List{elements});
        default:
          auto expr = readForm();
          elements.push_back(expr);
          continue;
        }
      }

      throw std::runtime_error("unbalanced parens");
    }
  }

  std::shared_ptr<Expr> readAtom() {
    auto token = advance();
    auto lexeme = token.lexeme;
    switch (token.type) {
    case TokenType::STRING:
      return std::make_shared<Expr>(String{token.lexeme});
    case TokenType::ATOM: {
      auto chr = lexeme[0];
      if (std::isdigit(chr) != 0) {
        return std::make_shared<Expr>(Int{parseUnsignedInteger(lexeme)});
      }
      if (chr == '-' && lexeme.size() >= 2 && std::isdigit(lexeme[1]) != 0) {
        auto num = parseUnsignedInteger(lexeme.substr(1));
        return std::make_shared<Expr>(Int{-num});
      }
      if (lexeme == "nil") {
        return std::make_shared<Expr>(Nil::NIL);
      }
      if (lexeme == "true") {
        return std::make_shared<Expr>(Bool::TRUE);
      }
      if (lexeme == "false") {
        return std::make_shared<Expr>(Bool::FALSE);
      }
      return std::make_shared<Expr>(Symbol{lexeme});
    }
    default:
      std::stringstream error;
      error << "invalid token type: " << tokenTypeName(token.type);
      throw std::runtime_error(error.str());
    }
  }

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

std::shared_ptr<Expr> readStr(const std::string &input);
