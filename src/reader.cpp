#include "reader.hpp"
#include "tokenizer.hpp"
// #include <iostream>
// #include <ostream>

namespace {
std::vector<Token> removeComments(const std::vector<Token> &tokens) {
  std::vector<Token> tokensSansComments;
  tokensSansComments.reserve(tokens.size());

  std::copy_if(tokens.begin(), tokens.end(),
               std::back_inserter(tokensSansComments),
               [](auto token) { return token.type != TokenType::COMMENT; });

  return tokensSansComments;
}
} // namespace

ExprPtr readStr(const std::string &input) {
  auto tokens = removeComments(tokenize(input));
  // for (auto token : tokens) {
  //   std::cerr << token.toString() << std::endl;
  // }
  if (tokens.empty()) {
    return std::make_shared<Expr>(Nil::NIL);
  }

  Reader reader(tokens);
  auto ret = reader.readForm();
  if (!reader.eof()) {
    throw std::runtime_error("leftover tokens");
  }
  return ret;
}
