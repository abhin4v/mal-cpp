#include "reader.hpp"
#include "tokenizer.hpp"
#include "types.hpp"

namespace {
void removeComments(std::vector<Token> &tokens) {

  auto newEnd = std::remove_if(tokens.begin(), tokens.end(), [](auto token) {
    return token.type == TokenType::COMMENT;
  });
  tokens.erase(newEnd, tokens.end());
}
} // namespace

Expr readStr(const std::string &input) {
  auto tokens = tokenize(input);
  removeComments(tokens);
  if (tokens.empty()) {
    return Nil::NIL;
  }

  Reader reader(tokens);
  auto ret = reader.readForm();
  if (!reader.eof()) {
    throw std::runtime_error("leftover tokens");
  }
  return ret;
}

Expr Reader::readForm() {
  auto oToken = peek();

  if (oToken.has_value()) {
    auto token = oToken.value();
    switch (token.type) {
    case TokenType::LEFT_PAREN:
      return List{readList(token, TokenType::RIGHT_PAREN)};

    case TokenType::LEFT_BRACKET: {
      const auto list = readList(token, TokenType::RIGHT_BRACKET);
      return Vector{{list.begin(), list.end()}};
    }

    case TokenType::LEFT_BRACE: {
      const auto list = readList(token, TokenType::RIGHT_BRACE);
      const std::vector<Expr> elements{list.begin(), list.end()};
      auto elementCount = elements.size();
      if (elementCount % 2 != 0) {
        throw std::runtime_error("odd number of elements in hash-map");
      }

      ExprMap map;
      map.reserve(elementCount / 2);

      for (auto i = 0; i < elementCount; i += 2) {
        map[elements[i]] = elements[i + 1];
      }

      return HashMap{map};
    }

    case TokenType::QUOTE: {
      advance();
      return Quote{readForm()};
    }

    case TokenType::BACKQUOTE: {
      advance();
      return QuasiQuote{readForm()};
    }

    case TokenType::TILDE: {
      advance();
      return Unquote{readForm()};
    }

    case TokenType::TILDE_AT: {
      advance();
      return SpliceUnquote{readForm()};
    }

    case TokenType::AT: {
      advance();
      return Deref{readForm()};
    }

    case TokenType::CARET: {
      advance();
      return MetaData{readForm(), readForm()};
    }

    default:
      return readAtom();
    }
  }

  throw std::runtime_error("no tokens");
}

ExprList Reader::readList(const Token &startDelimToken,
                          TokenType endDelimTokenType) {
  expect(startDelimToken);
  ExprList elements;

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

static const auto NIL_LEXEME = "nil";
static const auto TRUE_LEXEME = "true";
static const auto FALSE_LEXEME = "false";
static const auto KEYWORD_SIGIL = ':';

Expr Reader::readAtom() {
  auto token = advance();
  auto lexeme = token.lexeme;
  switch (token.type) {
  case TokenType::STRING:
    return String{token.lexeme};
  case TokenType::ATOM: {
    auto chr = lexeme[0];
    if (std::isdigit(chr) != 0) {
      return Int{parseUnsignedInteger(lexeme)};
    }
    if (chr == '-' && lexeme.size() >= 2 && std::isdigit(lexeme[1]) != 0) {
      auto num = parseUnsignedInteger(lexeme.substr(1));
      return Int{-num};
    }
    if (lexeme == NIL_LEXEME) {
      return Nil::NIL;
    }
    if (lexeme == TRUE_LEXEME) {
      return Bool::TRUE;
    }
    if (lexeme == FALSE_LEXEME) {
      return Bool::FALSE;
    }
    if (chr == KEYWORD_SIGIL) {
      return Keyword{lexeme.substr(1)};
    }
    return Symbol{lexeme};
  }
  default:
    std::stringstream error;
    error << "invalid token type: " << tokenTypeName(token.type);
    throw std::runtime_error(error.str());
  }
}
