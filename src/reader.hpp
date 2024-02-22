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

  ExprPtr readForm() {
    auto oToken = peek();

    if (oToken.has_value()) {
      auto token = oToken.value();
      switch (token.type) {
      case TokenType::LEFT_PAREN:
        return std::make_shared<Expr>(
            List{readList(token, TokenType::RIGHT_PAREN)});

      case TokenType::LEFT_BRACKET:
        return std::make_shared<Expr>(
            Vector{readList(token, TokenType::RIGHT_BRACKET)});

      case TokenType::LEFT_BRACE: {
        auto elements = readList(token, TokenType::RIGHT_BRACE);
        auto elementCount = elements.size();
        if (elementCount % 2 != 0) {
          throw std::runtime_error("odd number of elements in hash-map");
        }

        std::unordered_map<ExprPtr, ExprPtr> map;
        map.reserve(elementCount / 2);

        for (auto i = 0; i < elementCount; i += 2) {
          map[elements[i]] = elements[i + 1];
        }

        return std::make_shared<Expr>(HashMap{map});
      }

      case TokenType::QUOTE: {
        advance();
        return std::make_shared<Expr>(Quote{readForm()});
      }

      case TokenType::BACKQUOTE: {
        advance();
        return std::make_shared<Expr>(QuasiQuote{readForm()});
      }

      case TokenType::TILDE: {
        advance();
        return std::make_shared<Expr>(Unquote{readForm()});
      }

      case TokenType::TILDE_AT: {
        advance();
        return std::make_shared<Expr>(SpliceUnquote{readForm()});
      }

      case TokenType::AT: {
        advance();
        return std::make_shared<Expr>(Deref{readForm()});
      }

      case TokenType::CARET: {
        advance();
        return std::make_shared<Expr>(MetaData{readForm(), readForm()});
      }

      default:
        return readAtom();
      }
    }

    throw std::runtime_error("no tokens");
  }

private:
  std::vector<ExprPtr> readList(const Token &startDelimToken,
                                TokenType endDelimTokenType) {
    expect(startDelimToken);
    std::vector<ExprPtr> elements;

    while (true) {
      auto oToken = peek();
      if (oToken.has_value()) {
        auto token = oToken.value();
        if (token.type == endDelimTokenType) {
          advance();
          return elements;
        }

        auto expr = readForm();
        elements.push_back(expr);
        continue;
      }

      throw std::runtime_error("unbalanced delimiters");
    }
  }

  ExprPtr readAtom() {
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
      if (chr == ':') {
        return std::make_shared<Expr>(Keyword{lexeme.substr(1)});
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

ExprPtr readStr(const std::string &input);
