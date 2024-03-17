#pragma once

#include <functional>
#include <numeric>
#include <string>
#include <variant>

#include "types.hpp"

class Hasher {
 public:
  size_t operator()(const Nil & /*nil1*/) const { return 0; }
  size_t operator()(const Int &num) const {
    const std::hash<std::int64_t> hasher;
    return hasher(num.value);
  }
  size_t operator()(const Bool &boolean) const {
    const std::hash<bool> hasher;
    return hasher(boolean == Bool::TRUE);
  }
  size_t operator()(const String &str) const {
    const std::hash<std::string> hasher;
    return hasher(str.value);
  }
  size_t operator()(const Keyword &kwd) const {
    const std::hash<std::string> hasher;
    return hasher(kwd.value);
  }
  size_t operator()(const Symbol &sym) const {
    const std::hash<std::string> hasher;
    return hasher(sym.value);
  }
  size_t operator()(const BuiltinFunction &func) const {
    const std::hash<std::string> strHasher;
    auto hash1 = strHasher(func.name);
    const std::hash<int> intHasher;
    auto hash2 = intHasher(func.arity);
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

  size_t operator()(const Atom &atom) const { return hashExpr(atom.value); }

  size_t operator()(const List &list) const {
    return std::accumulate(
        list.value.begin(), list.value.end(), 0,
        [&](size_t hash, const auto &element) { return hash ^ (hashExpr(element) << 1U); });
  }
  size_t operator()(const Vector &vector) const {
    return std::accumulate(
        vector.value.begin(), vector.value.end(), 0,
        [&](size_t hash, const auto &element) { return hash ^ (hashExpr(element) << 1U); });
  }
  size_t operator()(const HashMap &hashmap) const {
    size_t hash{0};
    for (const auto &[key, value] : hashmap.value) {
      hash = hash ^ (hashExpr(key) << 1U);
      hash = hash ^ (hashExpr(value) << 1U);
    }
    return hash;
  }

 private:
  [[nodiscard]] size_t hashExpr(const Expr &expr) const { return std::visit(*this, expr); }
};

size_t exprHash(const Expr &expr);
