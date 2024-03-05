#pragma once

#include <cstdint>
#include <span>
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

struct BuiltinFunction;

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

struct Atom;

using Expr = std::variant<Nil, Int, Bool, String, Symbol, Keyword, List, Vector,
                          HashMap, Quote, QuasiQuote, Unquote, SpliceUnquote,
                          Deref, MetaData, BuiltinFunction, Lambda, Atom>;

using ExprPtr = std::shared_ptr<Expr>;

using ExprPtrVec = std::vector<ExprPtr>;

using ExprPtrSpan = std::span<ExprPtr, std::dynamic_extent>;

struct ExprPtrHash {
  size_t operator()(const ExprPtr &key) const;
};

struct ExprPtrEquals {
  bool operator()(const ExprPtr &key1, const ExprPtr &key2) const;
};

using ExprPtrMap =
    std::unordered_map<ExprPtr, ExprPtr, ExprPtrHash, ExprPtrEquals>;

using ExprFunction = std::function<ExprPtr(const ExprPtrSpan &)>;

using UnaryFunction = std::function<ExprPtr(ExprPtr)>;

using UnaryBooleanFunction = std::function<bool(ExprPtr)>;

using BinaryIntFunction =
    std::function<std::int64_t(std::int64_t, std::int64_t)>;

using VariadicIntFunction =
    std::function<std::int64_t(const std::vector<std::int64_t> &)>;

struct BuiltinFunction {
  std::string name;
  int arity;
  ExprFunction value;
};

class Env;

struct Lambda {
  std::vector<std::string> params;
  bool variadic;
  ExprPtr body;
  std::shared_ptr<Env> env;
};

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

struct Atom {
  ExprPtr value;
};

ExprPtr makeBuiltinFunction(std::string name, int arity, ExprFunction func);
ExprPtr makeBuiltinFunction(const std::string &name, UnaryFunction func);
ExprPtr makeBuiltinFunction(const std::string &name, UnaryBooleanFunction func);
ExprPtr makeBuiltinFunction(const std::string &name, BinaryIntFunction func);
ExprPtr makeBuiltinFunction(const std::string &name, VariadicIntFunction func);

ExprPtr nilPtr();
ExprPtr truePtr();
ExprPtr falsePtr();

void checkArgCount(size_t argCount, size_t expected);
void checkMinArgCount(size_t argCount, size_t expected);
std::int64_t intVal(const ExprPtr &exprPtr);
