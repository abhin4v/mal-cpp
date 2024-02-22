#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
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

struct Keyword {
  std::string value;
};

struct List;

struct Vector;

struct HashMap;

using Expr = std::variant<Nil, Int, Bool, String, Symbol, Keyword, List, Vector,
                          HashMap>;

using ExprPtr = std::shared_ptr<Expr>;

struct List {
  std::vector<ExprPtr> value{};
};

struct Vector {
  std::vector<ExprPtr> value{};
};

struct HashMap {
  std::unordered_map<ExprPtr, ExprPtr> value{};
};
