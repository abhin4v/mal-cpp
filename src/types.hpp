#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct String;
struct Int;
struct Symbol;
struct List;

using Expr = std::variant<String, Int, Symbol, List>;

struct String {
  std::string value;
};

struct Int {
  std::int64_t value;
};

struct Symbol {
  std::string value;
};

struct List {
  std::vector<std::shared_ptr<Expr>> value{};
};
