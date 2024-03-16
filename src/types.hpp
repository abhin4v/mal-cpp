#pragma once

#include "errors.hpp"
#include <functional>
#include <immer/flex_vector.hpp>
#include <immer/map.hpp>
#include <immer/vector.hpp>
#include <list>
#include <memory>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <variant>
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
                          Box<Vector>, Box<HashMap>, Box<BuiltinFunction>,
                          Box<Lambda>, Box<Atom>>;

using ExprPtr = Box<Expr>;

using ExprVec = immer::vector<Expr>;

using ExprList = immer::flex_vector<Expr>;

struct ExprHash {
  size_t operator()(const Expr &key) const;
};

struct ExprEquals {
  bool operator()(const Expr &key1, const Expr &key2) const;
};

using ExprMap = immer::map<Expr, Expr, ExprHash, ExprEquals>;

using ExprFunction = std::function<Expr(const ExprList &)>;

template <typename T, typename R>
using UnaryFunction = std::function<R(const T &)>;

template <typename T> using UnaryExprFunction = UnaryFunction<Expr, T>;

template <typename T, typename R>
using BinaryFunction = std::function<R(const T &, const T &)>;

template <typename T> using BinaryIntFunction = BinaryFunction<std::int64_t, T>;

template <typename T> using BinaryExprFunction = BinaryFunction<Expr, T>;

template <typename T, typename R>
using VariadicFunction = std::function<R(const immer::vector<T> &)>;

template <typename T>
using VariadicIntFunction = VariadicFunction<std::int64_t, T>;

struct BuiltinFunction {
  std::string name;
  int arity;
  ExprFunction value;
};

class Env;

using ParamNames = immer::vector<std::string>;

struct Lambda {
  ParamNames params;
  bool variadic;
  bool macro;
  Expr body;
  std::shared_ptr<Env> env;
  Expr metadata = Nil::NIL;
};

struct List {
  ExprList value{};
  Expr metadata = Nil::NIL;
  explicit List(ExprList value) : value(std::move(value)) {}
};

struct Vector {
  ExprVec value{};
  Expr metadata = Nil::NIL;
  explicit Vector(ExprVec value) : value(std::move(value)) {}
};

struct HashMap {
  ExprMap value{};
  Expr metadata = Nil::NIL;
  explicit HashMap(ExprMap value) : value(std::move(value)) {}
};

struct Atom {
  Expr value;
};

HashMap makeHashMap(const ExprList &elements);

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
