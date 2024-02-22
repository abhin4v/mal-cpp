#include "reader.hpp"
#include "tokenizer.hpp"
// #include <iostream>
// #include <ostream>

ExprPtr readStr(const std::string &input) {
  auto tokens = tokenize(input);
  // for (auto token : tokens) {
  //   std::cerr << token.toString() << std::endl;
  // }

  Reader reader(tokens);
  auto ret = reader.readForm();
  if (!reader.eof()) {
    throw std::runtime_error("leftover tokens");
  }
  return ret;
}
