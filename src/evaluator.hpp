#pragma once

#include "types.hpp"

ExprPtr eval(const Env &, const ExprPtr &);

class Evaluator {
private:
  Env env;

public:
  explicit Evaluator(const Env &env) : env(env) {}
  Expr operator()(const Nil &nil) const { return nil; }
  Expr operator()(const Int &num) const { return num; }
  Expr operator()(const Bool &boolean) const { return boolean; }
  Expr operator()(const String &str) const { return str; }
  Expr operator()(const Keyword &kwd) const { return kwd; }
  Expr operator()(const BuiltinFunction &func) const { return func; }
  Expr operator()(const Quote &expr) const { return expr; }
  Expr operator()(const QuasiQuote &expr) const { return expr; }
  Expr operator()(const Unquote &expr) const { return expr; }
  Expr operator()(const SpliceUnquote &expr) const { return expr; }
  Expr operator()(const Deref &expr) const { return expr; }

  Expr operator()(const MetaData &expr) const {
    return std::visit(*this, *(expr.value));
  }

  Expr operator()(const Symbol &sym) const {
    const auto key = sym.value;
    if (env.contains(key)) {
      return env.at(key);
    }

    throw std::runtime_error("symbol not found: " + key);
  }

  Expr operator()(const List &list) const {
    if (list.value.empty()) {
      return list;
    }

    ExprPtrVec elements;
    elements.reserve(list.value.size());
    for (const auto &elementPtr : list.value) {
      elements.push_back(eval(env, elementPtr));
    }
    return List{elements};
  }

  Expr operator()(const Vector &vector) const {
    if (vector.value.empty()) {
      return vector;
    }

    ExprPtrVec elements;
    elements.reserve(vector.value.size());
    for (const auto &elementPtr : vector.value) {
      elements.push_back(eval(env, elementPtr));
    }
    return Vector{elements};
  }

  Expr operator()(const HashMap &hashmap) const {
    if (hashmap.value.empty()) {
      return hashmap;
    }

    ExprPtrMap map;
    for (const auto &[key, value] : hashmap.value) {
      map[eval(env, key)] = eval(env, value);
    }
    return HashMap{map};
  }
};
