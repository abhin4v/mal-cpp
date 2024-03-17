#pragma once

#include <immer/flex_vector.hpp>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>

#include "constants.hpp"
#include "printer.hpp"
#include "types.hpp"

class Env {
 private:
  using EnvPtr = std::shared_ptr<Env>;
  EnvPtr outer;
  std::unordered_map<std::string, ExprPtr> data;

 public:
  explicit Env(EnvPtr outer) : outer(std::move(outer)) {}

  void set(const std::string &name, const Expr &expr) {
    if (const auto *list = std::get_if<Box<List>>(&expr)) {
      data.insert_or_assign(name,
                            Box{Expr{List{(*list)->value.push_front(this->get(LIST_FN_SYM))}}});
      return;
    }
    data.insert_or_assign(name, Box(expr));
  }

  [[nodiscard]] std::optional<ExprPtr> find(const std::string &name) const {
    const auto *env = this;
    while (env != nullptr) {
      if (env->data.contains(name)) {
        return std::make_optional(env->data.at(name));
      }
      env = (env->outer).get();
    }
    return std::nullopt;
  }

  [[nodiscard]] ExprPtr get(const std::string &name) const {
    auto val = find(name);
    if (val.has_value()) {
      return val.value();
    }
    throw MALError("'" + name + "' not found");
  }
};

using EnvPtr = std::shared_ptr<Env>;
