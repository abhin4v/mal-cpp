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

struct Quote;

struct QuasiQuote;

struct Unquote;

struct SpliceUnquote;

struct Deref;

struct MetaData;

using Expr =
    std::variant<Nil, Int, Bool, String, Symbol, Keyword, List, Vector, HashMap,
                 Quote, QuasiQuote, Unquote, SpliceUnquote, Deref, MetaData>;

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

struct Quote {
  ExprPtr value;
};

struct QuasiQuote {
  ExprPtr value;
};

struct Unquote {
  ExprPtr value;
};

struct SpliceUnquote {
  ExprPtr value;
};

struct Deref {
  ExprPtr value;
};

struct MetaData {
  ExprPtr meta;
  ExprPtr value;
};
