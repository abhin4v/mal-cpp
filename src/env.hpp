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
  using EnvPtr = std::shared_ptr<Env>;
  EnvPtr outer;
  std::unordered_map<std::string, ExprPtr> data;

public:
  explicit Env(EnvPtr outer) : outer(std::move(outer)) {}

  void set(const std::string &name, const ExprPtr &exprPtr) {
    if (data.contains(name)) {
      data[name] = exprPtr;
    } else {
      data.emplace(name, exprPtr);
    }
  }

  void set(std::string &&name, const ExprPtr &exprPtr) {
    if (data.contains(name)) {
      data[std::move(name)] = exprPtr;
    } else {
      data.emplace(std::move(name), exprPtr);
    }
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
    throw std::runtime_error("'" + name + "' not found");
  }

  //   static void dump(const EnvPtr &env) {
  //     std::cerr << "----------- Env Dump Start ------------" << std::endl;
  //     doDump(env);
  //     std::cerr << "-----------  Env Dump End  ------------" << std::endl;
  //   }

  // private:
  //   static void doDump(const EnvPtr &env) {
  //     for (const auto &[key, value] : env->data) {
  //       std::cerr << key << " = " << prStr(value, true) << std::endl;
  //     }
  //     if (env->data.empty()) {
  //       std::cerr << "EMPTY" << std::endl;
  //     }

  //     if (env->outer != nullptr) {

  //       std::cerr << "OUTER" << std::endl;
  //       doDump(env->outer);
  //     }
  //   }
};

using EnvPtr = std::shared_ptr<Env>;
