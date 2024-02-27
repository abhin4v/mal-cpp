#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

enum class Nil { NIL };

struct Int {
  std::int64_t value;
};

enum class Bool { TRUE, FALSE };

struct String {
  std::string value;
  explicit String(std::string value) : value(std::move(value)) {}
};

struct Symbol {
  std::string value;
  explicit Symbol(std::string value) : value(std::move(value)) {}
};

struct Keyword {
  std::string value;
  explicit Keyword(std::string value) : value(std::move(value)) {}
};

using BinaryFunction = std::function<std::int64_t(std::int64_t, std::int64_t)>;

struct BuiltinFunction {
  std::string name;
  BinaryFunction value;
};

struct Lambda;

struct List;

struct Vector;

struct HashMap;

struct Quote;

struct QuasiQuote;

struct Unquote;

struct SpliceUnquote;

struct Deref;

struct MetaData;

using Expr = std::variant<Nil, Int, Bool, String, Symbol, Keyword, List, Vector,
                          HashMap, Quote, QuasiQuote, Unquote, SpliceUnquote,
                          Deref, MetaData, BuiltinFunction, Lambda>;

using ExprPtr = std::shared_ptr<const Expr>;

using ExprPtrVec = std::vector<ExprPtr>;

using ExprPtrMap = std::unordered_map<ExprPtr, ExprPtr>;

struct List {
  ExprPtrVec value{};
  explicit List(ExprPtrVec value) : value(std::move(value)) {}
};

struct Vector {
  ExprPtrVec value{};
  explicit Vector(ExprPtrVec value) : value(std::move(value)) {}
};

struct HashMap {
  ExprPtrMap value{};
  explicit HashMap(ExprPtrMap value) : value(std::move(value)) {}
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

class Env;

struct Lambda {
  std::vector<std::string> params;
  ExprPtr body;
  std::shared_ptr<Env> env;
};

std::shared_ptr<Expr> makeBuiltinFunction(std::string name,
                                          BinaryFunction func);

ExprPtr nilPtr();
ExprPtr truePtr();
ExprPtr falsePtr();
