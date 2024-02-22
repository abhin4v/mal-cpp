#include "linenoise.h"
#include "tokenizer.hpp"
#include <iostream>
#include <sstream>

std::vector<Token> READ(const std::string &input) { return tokenize(input); }

std::vector<Token> EVAL(const std::vector<Token> &ast) { return ast; }

std::string PRINT(const std::vector<Token> &value) {
  std::stringstream buffer;
  for (auto const &token : value) {
    buffer << "Token<" << tokenTypeName(token.type) << " " << token.lexeme
           << ">" << std::endl;
  }
  return buffer.str();
}

std::string rep(const std::string &input) { return PRINT(EVAL(READ(input))); }

static const auto HISTORY_FILE = ".repl_history";
static const auto HISTORY_MAX_LEN = 1000;

int main() {
  linenoiseHistoryLoad(HISTORY_FILE);
  linenoiseHistorySetMaxLen(HISTORY_MAX_LEN);

  while (true) {
    auto *line = linenoise("user> ");
    if (line == nullptr) {
      break;
    }

    linenoiseHistoryAdd(line);          /* Add to the history. */
    linenoiseHistorySave(HISTORY_FILE); /* Save the history on disk. */

    auto output = rep(std::string(line));
    std::cout << output << std::endl;

    linenoiseFree(line);
  }
}
