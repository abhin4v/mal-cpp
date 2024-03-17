#include "tokenizer.hpp"

static constexpr std::array<const char *, 16> TOKEN_TYPE_NAMES = {
    "UD", "~", "@", "~@", "[", "]", "(", ")", "{", "}", "'", "`", "^", "COMMENT", "ATOM", "STRING"};

const char *tokenTypeName(TokenType tokenType) {
  return const_cast<char *>(TOKEN_TYPE_NAMES.at(static_cast<int>(tokenType)));
}

std::vector<Token> Tokenizer::tokenize() {
  while (!eof()) {
    auto chr = peek().value();  // NOLINT(bugprone-unchecked-optional-access)
    switch (chr) {
      case '\n':
      case '\r':
      case ',':
      case ' ':
      case '\t':
        advance();
        tokenizeWhitespace(chr);
        break;
      case '~': {
        auto next = peek(1);
        if (next.has_value() && next.value() == '@' && prevTokenType != TokenType::STRING &&
            prevTokenType != TokenType::ATOM && prevTokenType != TokenType::COMMENT) {
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
        tokenizeSpecialChar(chr);
        break;
      case ';':
        advance();
        tokenizeComment(chr);
        break;
      case '\\':
        advance();
        tokenizeEscape(chr);
        break;
      case '"':
        advance();
        tokenizeString(chr);
        break;
      default:
        advance();
        switch (prevTokenType) {
          case TokenType::COMMENT:
          case TokenType::STRING:
          case TokenType::ATOM:
            bufferChar(chr);
            break;
          default:
            assertEmptyBuffer();
            bufferChar(chr);
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
        throw TokenizerError("unbalanced string");
      default:
        throw TokenizerError("leftover input");
    }
  } else if (prevTokenType == TokenType::STRING) {
    throw TokenizerError("unbalanced string");
  }

  return tokens;
}

void Tokenizer::tokenizeWhitespace(char chr) {
  switch (prevTokenType) {
    case TokenType::COMMENT:
      if (chr == '\n' || chr == '\r') {
        addTokenFromBuffer(TokenType::COMMENT);
        prevTokenType = TokenType::UNDEFINED;
        break;
      }
    case TokenType::STRING:
      bufferChar(chr);
      break;
    case TokenType::ATOM:
      addTokenFromBuffer(TokenType::ATOM);
    default:
      prevTokenType = TokenType::UNDEFINED;
      break;
  }
}

void Tokenizer::tokenizeSpecialChar(char chr) {
  switch (prevTokenType) {
    case TokenType::COMMENT:
    case TokenType::STRING:
      bufferChar(chr);
      break;
    case TokenType::ATOM:
      if (chr == '~' || chr == '@' || chr == '^') {
        bufferChar(chr);
        break;
      }
      addTokenFromBuffer(TokenType::ATOM);
    default:
      auto tokenType = charTokenType(chr);
      addToken(tokenType, std::string(1, chr));
      prevTokenType = tokenType;
      break;
  }
}

void Tokenizer::tokenizeComment(char chr) {
  switch (prevTokenType) {
    case TokenType::COMMENT:
    case TokenType::STRING:
      bufferChar(chr);
      break;
    case TokenType::ATOM:
      addTokenFromBuffer(TokenType::ATOM);
    default:
      assertEmptyBuffer();
      prevTokenType = TokenType::COMMENT;
      break;
  }
}

void Tokenizer::tokenizeEscape(char chr) {
  switch (prevTokenType) {
    case TokenType::STRING: {
      auto next = peek();
      if (next.has_value()) {
        auto val = next.value();
        if (val == '"' || val == '\\') {
          advance();
          bufferChar(val);
        }
        if (val == 'n') {
          advance();
          bufferChar('\n');
        }
      } else {
        bufferChar(chr);
      }
      break;
    }
    case TokenType::ATOM:
    case TokenType::COMMENT:
      bufferChar(chr);
      break;
    default:
      assertEmptyBuffer();
      bufferChar(chr);
      prevTokenType = TokenType::ATOM;
      break;
  }
}

void Tokenizer::tokenizeString(char chr) {
  switch (prevTokenType) {
    case TokenType::COMMENT:
      bufferChar(chr);
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
}

std::vector<Token> tokenize(const std::string &input) {
  Tokenizer tokenizer(input);
  return tokenizer.tokenize();
}
