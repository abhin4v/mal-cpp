#include <iostream>
#include <ostream>
#include <string>

std::string READ(const std::string& input) {
  return input;
}

std::string EVAL(const std::string& ast) {
  return ast;
}

std::string PRINT(const std::string& value) {
  return value;
}

std::string rep(const std::string& input) {
  return PRINT(EVAL(READ(input)));
}

int main() {
  std::string input;

  std::cout << "user> ";
  while(std::getline(std::cin, input)) {
    auto output = rep(input);
    std::cout << output << std::endl;
    std::cout << "user> ";
  }
}
