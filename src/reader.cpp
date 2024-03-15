#include "reader.hpp"
#include "constants.hpp"
#include "errors.hpp"
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
    throw ReaderError("leftover tokens: " + reader.advance().toString());
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
      try {
        return makeHashMap(list);
      } catch (const MALError &err) {
        throw ReaderError(err.what());
      }
    }

    case TokenType::QUOTE: {
      advance();
      return List{{Symbol{QUOTE_FORM_SYM}, readForm()}};
    }

    case TokenType::BACKQUOTE: {
      advance();
      return List{{Symbol{QUASIQUOTE_FORM_SYM}, readForm()}};
    }

    case TokenType::TILDE: {
      advance();
      return List{{Symbol{UNQUOTE_FORM_SYM}, readForm()}};
    }

    case TokenType::TILDE_AT: {
      advance();
      return List{{Symbol{SPLICE_UNQUOTE_FORM_SYM}, readForm()}};
    }

    case TokenType::AT: {
      advance();
      return List{{Symbol{DEREF_FN_SYM}, readForm()}};
    }

    case TokenType::CARET: {
      advance();
      auto meta = readForm();
      auto data = readForm();
      return List{{Symbol{WITH_META_FN_SYM}, std::move(data), std::move(meta)}};
    }

    default:
      return readAtom();
    }
  }

  throw ReaderError("no tokens");
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

    throw ReaderError("unbalanced delimiters");
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
    throw ReaderError(error.str());
  }
}
