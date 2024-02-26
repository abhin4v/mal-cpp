#include "reader.hpp"
#include "tokenizer.hpp"

namespace {
void removeComments(std::vector<Token> &tokens) {

  auto newEnd = std::remove_if(tokens.begin(), tokens.end(), [](auto token) {
    return token.type == TokenType::COMMENT;
  });
  tokens.erase(newEnd, tokens.end());
}
} // namespace

ExprPtr readStr(const std::string &input) {
  auto tokens = tokenize(input);
  removeComments(tokens);
  if (tokens.empty()) {
    return nilPtr();
  }

  Reader reader(tokens);
  auto ret = reader.readForm();
  if (!reader.eof()) {
    throw std::runtime_error("leftover tokens");
  }
  return ret;
}

ExprPtr Reader::readForm() {
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

      ExprPtrMap map;
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

ExprPtrVec Reader::readList(const Token &startDelimToken,
                            TokenType endDelimTokenType) {
  expect(startDelimToken);
  ExprPtrVec elements;

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

ExprPtr Reader::readAtom() {
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
    if (lexeme == NIL_LEXEME) {
      return nilPtr();
    }
    if (lexeme == TRUE_LEXEME) {
      return std::make_shared<Expr>(Bool::TRUE);
    }
    if (lexeme == FALSE_LEXEME) {
      return std::make_shared<Expr>(Bool::FALSE);
    }
    if (chr == KEYWORD_SIGIL) {
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
