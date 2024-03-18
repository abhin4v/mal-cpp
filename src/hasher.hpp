#pragma once

#include <functional>
#include <numeric>
#include <string>
#include <variant>

#include "types.hpp"

class Hasher {
 public:
  size_t operator()(const Nil & /*nil1*/) const { return 0; }
  size_t operator()(const Int &num) const { return std::hash<std::int64_t>()(num.value); }
  size_t operator()(const Bool &boolean) const { return std::hash<bool>()(boolean == Bool::TRUE); }
  size_t operator()(const String &str) const { return std::hash<std::string>()(str.value); }
  size_t operator()(const Keyword &kwd) const { return std::hash<std::string>()(kwd.value); }
  size_t operator()(const Symbol &sym) const { return std::hash<std::string>()(sym.value); }
  size_t operator()(const Atom &atom) const { return hashExpr(atom.value); }

  size_t operator()(const BuiltinFunction &func) const {
    auto hash1 = std::hash<std::string>()(func.name);
    auto hash2 = std::hash<int>()(func.arity);
    return hash1 ^ (hash2 << 1U);
  }
  size_t operator()(const Lambda &func) const {
    const std::hash<std::string> strHasher;
    const size_t hash =
        std::accumulate(func.params.begin(), func.params.end(), 0,
                        [&](size_t hash, auto param) { return hash ^ (strHasher(param) << 1U); });

    const auto bodyHash = hashExpr(func.body);
    return hash ^ (bodyHash << 1U);
  }

  size_t operator()(const List &list) const {
    return std::accumulate(list.begin(), list.end(), 0, [&](size_t hash, const auto &element) {
      return hash ^ (hashExpr(element) << 1U);
    });
  }
  size_t operator()(const Vector &vector) const {
    return std::accumulate(vector.begin(), vector.end(), 0, [&](size_t hash, const auto &element) {
      return hash ^ (hashExpr(element) << 1U);
    });
  }
  size_t operator()(const HashMap &hashmap) const {
    size_t hash{0};
    for (const auto &[key, value] : hashmap) {
      hash = hash ^ (hashExpr(key) << 1U);
      hash = hash ^ (hashExpr(value) << 1U);
    }
    return hash;
  }

  static size_t exprHash(const Expr &expr) {
    Hasher hasher;
    return std::visit(hasher, expr);
  }

 private:
  [[nodiscard]] size_t hashExpr(const Expr &expr) const { return std::visit(*this, expr); }
};
