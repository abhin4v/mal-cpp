#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

template <typename T> class Box {
public:
  Box() = delete;
  // NOLINTNEXTLINE(google-explicit-constructor, hicpp-explicit-conversions)
  Box(T value) // cppcheck-suppress noExplicitConstructor
      : ptr(std::make_shared<T>(std::move(value))) {}

  template <class U>
  using enabler = std::enable_if_t<std::is_same_v<std::decay_t<U>, T>>;
  template <typename U, enabler<U>>
  // NOLINTBEGIN(google-explicit-constructor, hicpp-explicit-conversions,
  // bugprone-forwarding-reference-overload)
  Box(U &&value) // cppcheck-suppress noExplicitConstructor
      : ptr(std::make_shared<U>(std::forward<U>(value))) {}
  // NOLINTEND(google-explicit-constructor, hicpp-explicit-conversions,
  // bugprone-forwarding-reference-overload)

  // NOLINTNEXTLINE(google-explicit-constructor, hicpp-explicit-conversions)
  operator T &() { return *ptr; }
  // NOLINTNEXTLINE(google-explicit-constructor, hicpp-explicit-conversions)
  operator const T &() const { return *ptr; }

  T *operator->() const { return &*ptr; }
  [[nodiscard]] T &getRaw() const { return *ptr; }

private:
  std::shared_ptr<T> ptr;
};

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

using Expr =
    std::variant<Nil, Int, Bool, String, Symbol, Keyword, Box<List>,
                 Box<Vector>, Box<HashMap>, Box<Quote>, Box<QuasiQuote>,
                 Box<Unquote>, Box<SpliceUnquote>, Box<Deref>, Box<MetaData>,
                 Box<BuiltinFunction>, Box<Lambda>, Box<Atom>>;

using ExprPtr = Box<Expr>;

using ExprVec = std::vector<Expr>;

using ExprSpan = std::span<Expr, std::dynamic_extent>;

struct ExprHash {
  size_t operator()(const Expr &key) const;
};

struct ExprEquals {
  bool operator()(const Expr &key1, const Expr &key2) const;
};

using ExprMap = std::unordered_map<Expr, Expr, ExprHash, ExprEquals>;

using ExprFunction = std::function<Expr(const ExprSpan &)>;

using UnaryFunction = std::function<Expr(const Expr &)>;

using UnaryBooleanFunction = std::function<bool(const Expr &)>;

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
  Expr body;
  std::shared_ptr<Env> env;
};

struct List {
  ExprVec value{};
  explicit List(ExprVec value) : value(std::move(value)) {}
};

struct Vector {
  ExprVec value{};
  explicit Vector(ExprVec value) : value(std::move(value)) {}
};

struct HashMap {
  ExprMap value{};
  explicit HashMap(ExprMap value) : value(std::move(value)) {}
};

struct Quote {
  Expr value;
};

struct QuasiQuote {
  Expr value;
};

struct Unquote {
  Expr value;
};

struct SpliceUnquote {
  Expr value;
};

struct Deref {
  Expr value;
};

struct MetaData {
  Expr meta;
  Expr value;
};

struct Atom {
  Expr value;
};

Expr makeBuiltinFunction(std::string name, int arity, ExprFunction func);
Expr makeBuiltinFunction(const std::string &name, UnaryFunction func);
Expr makeBuiltinFunction(const std::string &name, UnaryBooleanFunction func);
Expr makeBuiltinFunction(const std::string &name, BinaryIntFunction func);
Expr makeBuiltinFunction(const std::string &name, VariadicIntFunction func);

void checkArgCount(size_t argCount, size_t expected);
void checkMinArgCount(size_t argCount, size_t expected);
std::int64_t intVal(const Expr &expr);
