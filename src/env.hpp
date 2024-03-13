#pragma once

#include "constants.hpp"
#include "printer.hpp"
#include "types.hpp"
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>

class Env {
private:
  using EnvPtr = std::shared_ptr<Env>;
  EnvPtr outer;
  std::unordered_map<std::string, ExprPtr> data;

public:
  explicit Env(EnvPtr outer) : outer(std::move(outer)) {}

  void set(const std::string &name, const Expr &expr) {
    if (const auto *list = std::get_if<Box<List>>(&expr)) {
      ExprList elements{this->get(LIST_FN_SYM)};
      elements.insert(elements.end(), (*list)->value.begin(),
                      (*list)->value.end());
      const Expr &listCopy = List{elements};
      data.insert_or_assign(name, Box(listCopy));
      return;
    }
    data.insert_or_assign(name, Box(expr));
  }

  [[nodiscard]] std::optional<ExprPtr> find(const std::string &name) const {
    const auto *env = this;
    while (env != nullptr) {
      if (env->data.contains(name)) {
        const auto &val = env->data.at(name);
        if (const auto *list = std::get_if<Box<List>>(&(val.getRaw()))) {
          const Expr &listCopy =
              List{ExprList{(*list)->value.begin(), (*list)->value.end()}};
          return std::make_optional(Box(listCopy));
        }
        return std::make_optional(val);
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
