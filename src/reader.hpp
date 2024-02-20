#pragma once

#include <cassert>
#include <string>
#include <vector>

enum class TokenType {
  UNDEFINED,
  TILDE,
  AT,
  TILDE_AT,
  LEFT_BRACKET,
  RIGHT_BRACKET,
  LEFT_PAREN,
  RIGHT_PAREN,
  LEFT_BRACE,
  RIGHT_BRACE,
  QUOTE,
  BACKQUOTE,
  CARET,
  COMMENT,
  ATOM,
  STRING,
};

struct Token {
  TokenType type;
  std::string lexeme;
  Token(TokenType t, const std::string &l) : type(t), lexeme(l) {}
};
const char *tokenTypeName(TokenType tokenType);

class Tokenizer {
private:
  std::string input;
  int position;
  std::vector<Token> tokens;
  TokenType prevTokenType;
  std::string buffer;

public:
  explicit Tokenizer(const std::string &input)
      : input(input), position(0), prevTokenType(TokenType::UNDEFINED),
        buffer(""){};

  std::vector<Token> tokenize();

private:
  std::optional<char> peek(int offset = 0) {
    if (position + offset >= input.size()) {
      return std::nullopt;
    }
    return std::make_optional(input[position + offset]);
  }

  void advance() {
    auto c = peek();
    if (c.has_value()) {
      position++;
    }
  }

  bool eof() { return !peek().has_value(); }

  static TokenType charTokenType(char c) {
    switch (c) {
    case '~':
      return TokenType::TILDE;
    case '@':
      return TokenType::AT;
    case '[':
      return TokenType::LEFT_BRACKET;
    case ']':
      return TokenType::RIGHT_BRACKET;
    case '{':
      return TokenType::LEFT_BRACE;
    case '}':
      return TokenType::RIGHT_BRACE;
    case '(':
      return TokenType::LEFT_PAREN;
    case ')':
      return TokenType::RIGHT_PAREN;
    case '\'':
      return TokenType::QUOTE;
    case '`':
      return TokenType::BACKQUOTE;
    case '^':
      return TokenType::CARET;
    default:
      std::string error("invalid character: ");
      error += c;
      throw std::runtime_error(error);
    }
  }

  void addTokenFromBuffer(TokenType tokenType) {
    addToken(tokenType, buffer);
    buffer.clear();
  }

  void addToken(TokenType tokenType, const std::string &lexeme) {
    assert(tokenType != TokenType::UNDEFINED && "Token type is undefined");
    tokens.emplace_back(tokenType, lexeme);
  }

  void bufferChar(char c) { buffer.push_back(c); }

  void assertEmptyBuffer() { assert(buffer.empty() && "Buffer is not empty"); }
};

std::vector<Token> tokenize(const std::string &input);
