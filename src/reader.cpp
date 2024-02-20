#include "reader.hpp"

static constexpr std::array<const char *, 16> TOKEN_TYPE_NAMES = {
    "UD", "~", "@", "~@", "[", "]",       "(",    ")",
    "{",  "}", "'", "`",  "^", "COMMENT", "ATOM", "STRING"};

const char *tokenTypeName(TokenType tokenType) {
  return (char *)TOKEN_TYPE_NAMES[static_cast<int>(tokenType)];
}

std::vector<Token> Tokenizer::tokenize() {
  while (!eof()) {
    auto c = peek().value();
    // std::cerr << "Prev token = " << tokenTypeName(prevTokenType)
    //           << ", current str = \"" << buffer << "\", char = \'" << c <<
    //           "\'" << std::endl;
    switch (c) {
    case '\n':
    case '\r':
    case ',':
    case ' ':
    case '\t':
      advance();
      switch (prevTokenType) {
      case TokenType::COMMENT:
        if (c == '\n' || c == '\r') {
          addTokenFromBuffer(TokenType::COMMENT);
          prevTokenType = TokenType::UNDEFINED;
          break;
        }
      case TokenType::STRING:
        bufferChar(c);
        break;
      case TokenType::ATOM:
        addTokenFromBuffer(TokenType::ATOM);
      default:
        prevTokenType = TokenType::UNDEFINED;
        break;
      }
      break;
    case '~':
      if (prevTokenType != TokenType::STRING &&
          prevTokenType != TokenType::ATOM &&
          prevTokenType != TokenType::COMMENT) {
        auto next = peek(1);
        if (next.has_value() && next.value() == '@') {
          advance();
          advance();
          addToken(TokenType::TILDE_AT, "~@");
          prevTokenType = TokenType::TILDE_AT;
          break;
        }
      }
    case '@':
    case '[':
    case ']':
    case '{':
    case '}':
    case '(':
    case ')':
    case '\'':
    case '`':
    case '^':
      advance();
      switch (prevTokenType) {
      case TokenType::COMMENT:
      case TokenType::STRING:
        bufferChar(c);
        break;
      case TokenType::ATOM:
        if (c == '~' || c == '@' || c == '^') {
          bufferChar(c);
          break;
        }
        addTokenFromBuffer(TokenType::ATOM);
      default:
        auto tokenType = charTokenType(c);
        addToken(tokenType, std::string(1, c));
        prevTokenType = tokenType;
        break;
      }
      break;
    case ';':
      advance();
      switch (prevTokenType) {
      case TokenType::COMMENT:
      case TokenType::STRING:
        bufferChar(c);
        break;
      case TokenType::ATOM:
        addTokenFromBuffer(TokenType::ATOM);
      default:
        assertEmptyBuffer();
        prevTokenType = TokenType::COMMENT;
        break;
      }
      break;
    case '\\':
      advance();
      switch (prevTokenType) {
      case TokenType::STRING: {
        auto next = peek();
        if (next.has_value() && next.value() == '"') {
          advance();
          bufferChar('"');
        } else {
          bufferChar(c);
        }
        break;
      }
      case TokenType::ATOM:
      case TokenType::COMMENT:
        bufferChar(c);
        break;
      default:
        assertEmptyBuffer();
        bufferChar(c);
        prevTokenType = TokenType::ATOM;
        break;
      }
      break;
    case '"':
      advance();
      switch (prevTokenType) {
      case TokenType::COMMENT:
        bufferChar(c);
        break;
      case TokenType::STRING:
        addTokenFromBuffer(TokenType::STRING);
        prevTokenType = TokenType::UNDEFINED;
        break;
      case TokenType::ATOM:
        addTokenFromBuffer(TokenType::ATOM);
        prevTokenType = TokenType::STRING;
        break;
      default:
        assertEmptyBuffer();
        prevTokenType = TokenType::STRING;
        break;
      }
      break;
    default:
      advance();
      switch (prevTokenType) {
      case TokenType::COMMENT:
      case TokenType::STRING:
      case TokenType::ATOM:
        bufferChar(c);
        break;
      default:
        assertEmptyBuffer();
        bufferChar(c);
        prevTokenType = TokenType::ATOM;
        break;
      }
      break;
    }
  }

  if (!buffer.empty()) {
    switch (prevTokenType) {
    case TokenType::COMMENT:
    case TokenType::ATOM:
      addTokenFromBuffer(prevTokenType);
      break;
    case TokenType::STRING:
      throw std::runtime_error("Unclosed string");
    default:
      throw std::runtime_error("Leftover input");
    }
  } else if (prevTokenType == TokenType::STRING) {
    throw std::runtime_error("Unclosed string");
  }

  return tokens;
}

std::vector<Token> tokenize(const std::string &input) {
  Tokenizer tokenizer(input);
  return tokenizer.tokenize();
}
