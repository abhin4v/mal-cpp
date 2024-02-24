#pragma once

#include <cassert>
#include <sstream>
#include <string>
#include <utility>
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

const char *tokenTypeName(TokenType tokenType);

struct Token {
  TokenType type;
  std::string lexeme;
  Token(TokenType type, std::string lexeme)
      : type(type), lexeme(std::move(lexeme)) {}

  [[nodiscard]] std::string toString() const {
    std::stringstream buffer;
    buffer << "Token<" << tokenTypeName(type) << " " << lexeme << ">";
    return buffer.str();
  }

  bool operator==(const Token &other) const {
    return type == other.type && lexeme == other.lexeme;
  }

  bool operator!=(const Token &other) const {
    return !(this->operator==(other));
  }
};

class Tokenizer {
private:
  std::string input;
  int position{0};
  std::vector<Token> tokens;
  TokenType prevTokenType{TokenType::UNDEFINED};
  std::string buffer;

public:
  explicit Tokenizer(std::string input) : input(std::move(input)){};

  std::vector<Token> tokenize();

private:
  void tokenizeWhitespace(char chr);

  void tokenizeSpecialChar(char chr);

  void tokenizeComment(char chr);

  void tokenizeEscape(char chr);

  void tokenizeString(char chr);

  std::optional<char> peek(int offset = 0) {
    if (position + offset >= input.size()) {
      return std::nullopt;
    }
    return std::make_optional(input[position + offset]);
  }

  void advance() {
    auto chr = peek();
    if (chr.has_value()) {
      position++;
    }
  }

  bool eof() { return !peek().has_value(); }

  static TokenType charTokenType(char chr) {
    switch (chr) {
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
      error += chr;
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

  void bufferChar(char chr) { buffer.push_back(chr); }

  void assertEmptyBuffer() { assert(buffer.empty() && "Buffer is not empty"); }
};

std::vector<Token> tokenize(const std::string &input);
