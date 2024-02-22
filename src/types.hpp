#pragma once

#include <cstdint>
#include <string>
#include <vector>

enum class Nil { NIL };

struct Int {
  std::int64_t value;
};

enum class Bool { TRUE, FALSE };

struct String {
  std::string value;
};

struct Symbol {
  std::string value;
};

struct List;

using Expr = std::variant<Nil, Int, Bool, String, Symbol, List>;

struct List {
  std::vector<std::shared_ptr<Expr>> value{};
};
