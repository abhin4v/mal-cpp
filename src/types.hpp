#pragma once

#include "errors.hpp"
#include <cstdint>
#include <list>
#include <span>
#include <stdexcept>
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

struct MetaData;

struct Atom;

using Expr = std::variant<Nil, Int, Bool, String, Symbol, Keyword, Box<List>,
                          Box<Vector>, Box<HashMap>, Box<MetaData>,
                          Box<BuiltinFunction>, Box<Lambda>, Box<Atom>>;

using ExprPtr = Box<Expr>;

using ExprVec = std::vector<Expr>;

using ExprList = std::list<Expr>;

struct ExprHash {
  size_t operator()(const Expr &key) const;
};

struct ExprEquals {
  bool operator()(const Expr &key1, const Expr &key2) const;
};

using ExprMap = std::unordered_map<Expr, Expr, ExprHash, ExprEquals>;

using ExprFunction = std::function<Expr(ExprList &)>;

template <typename I, typename O>
using UnaryFunction = std::function<O(const I &)>;

template <typename T> using UnaryExprFunction = UnaryFunction<Expr, T>;

template <typename I, typename O>
using BinaryFunction = std::function<O(const I &, const I &)>;

template <typename T> using BinaryIntFunction = BinaryFunction<std::int64_t, T>;

template <typename T> using BinaryExprFunction = BinaryFunction<Expr, T>;

template <typename I, typename O>
using VariadicFunction = std::function<O(const std::vector<I> &)>;

template <typename T>
using VariadicIntFunction = VariadicFunction<std::int64_t, T>;

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
  ExprList value{};
  explicit List(ExprList value) : value(std::move(value)) {}
};

struct Vector {
  ExprVec value{};
  explicit Vector(ExprVec value) : value(std::move(value)) {}
};

struct HashMap {
  ExprMap value{};
  explicit HashMap(ExprMap value) : value(std::move(value)) {}
};

struct MetaData {
  Expr meta;
  Expr value;
};

struct Atom {
  Expr value;
};

Expr makeBuiltinFunction(std::string name, int arity, ExprFunction func);

template <typename T>
Expr makeBuiltinFunction(const std::string &name, UnaryExprFunction<T> func);

template <typename T>
Expr makeBuiltinFunction(const std::string &name, BinaryIntFunction<T> func);

template <typename T>
Expr makeBuiltinFunction(const std::string &name, BinaryExprFunction<T> func);

Expr makeBuiltinFunction(const std::string &name,
                         VariadicIntFunction<std::int64_t> func);

void checkArgCount(size_t argCount, size_t expected);
void checkMinArgCount(size_t argCount, size_t expected);
std::int64_t intVal(const Expr &expr);

class WrongTypeError : public MALError {
public:
  explicit WrongTypeError(const std::string &expected, const Expr &actual);
};

class WrongArgCountError : public MALError {
public:
  explicit WrongArgCountError(const std::string &expected, size_t &actual)
      : MALError("wrong argument count: expected " + expected +
                 " arguments, got: " + std::to_string(actual)) {}
};
