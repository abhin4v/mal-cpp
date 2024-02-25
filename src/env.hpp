#pragma once

#include "printer.hpp"
#include "types.hpp"
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>

class Env {
private:
  std::unique_ptr<Env> outer;
  std::unordered_map<std::string, ExprPtr> data;

public:
  explicit Env(std::unique_ptr<Env> outer) : outer(std::move(outer)) {}

  void set(std::string name, const ExprPtr &exprPtr) {
    data.emplace(std::move(name), exprPtr);
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

    throw std::runtime_error("unknown binding name: " + name);
  }
};
