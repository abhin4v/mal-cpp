#include "core.hpp"

std::unordered_map<std::string, Expr> Core::makeBuiltins() noexcept {
  std::unordered_map<std::string, Expr> builtins;
  // arithmetic functions
  builtins.emplace(
      "+", makeBuiltinFunction("+", [](const std::vector<std::int64_t> &args) {
        return std::accumulate(args.begin(), args.end(), 0LL);
      }));
  builtins.emplace(
      "*", makeBuiltinFunction("*", [](const std::vector<std::int64_t> &args) {
        return std::accumulate(args.begin(), args.end(), 1LL,
                               std::multiplies<>());
      }));
  builtins.emplace("-", makeBuiltinFunction<std::int64_t>(
                            "-", [](std::int64_t arg1, std::int64_t arg2) {
                              return arg1 - arg2;
                            }));
  builtins.emplace("/", makeBuiltinFunction<std::int64_t>(
                            "/", [](std::int64_t arg1, std::int64_t arg2) {
                              return arg1 / arg2;
                            }));

  builtins.emplace("<", makeBuiltinFunction<bool>(
                            "<", [](std::int64_t arg1, std::int64_t arg2) {
                              return arg1 < arg2;
                            }));
  builtins.emplace(">", makeBuiltinFunction<bool>(
                            ">", [](std::int64_t arg1, std::int64_t arg2) {
                              return arg1 > arg2;
                            }));
  builtins.emplace("<=", makeBuiltinFunction<bool>(
                             "<=", [](std::int64_t arg1, std::int64_t arg2) {
                               return arg1 <= arg2;
                             }));
  builtins.emplace(">=", makeBuiltinFunction<bool>(
                             ">=", [](std::int64_t arg1, std::int64_t arg2) {
                               return arg1 >= arg2;
                             }));
  builtins.emplace("=", makeBuiltinFunction<bool>(
                            "=", [](const Expr &arg1, const Expr &arg2) {
                              return exprEquals(arg1, arg2);
                            }));

  // print to string functions
  builtins.emplace("pr-str",
                   makeBuiltinFunction("pr-str", -1, [](const ExprList &args) {
                     return String{toString(args, true, " ")};
                   }));
  builtins.emplace("str",
                   makeBuiltinFunction("str", -1, [](const ExprList &args) {
                     return String{toString(args, false, "")};
                   }));

  // print to stdout functions
  builtins.emplace("prn",
                   makeBuiltinFunction("prn", -1, [](const ExprList &args) {
                     std::cout << toString(args, true, " ") << std::endl;
                     return Nil::NIL;
                   }));
  builtins.emplace("println",
                   makeBuiltinFunction("println", -1, [](const ExprList &args) {
                     std::cout << toString(args, false, " ") << std::endl;
                     return Nil::NIL;
                   }));

  // collection functions
  builtins.emplace("list?",
                   makeBuiltinFunction<bool>("list?", [](const Expr &arg) {
                     return std::holds_alternative<Box<List>>(arg);
                   }));
  builtins.emplace("empty?",
                   makeBuiltinFunction<bool>("empty?", [](const Expr &arg) {
                     return collectionCount(arg) == 0;
                   }));
  builtins.emplace("count",
                   makeBuiltinFunction<Expr>("count", [](const Expr &arg) {
                     return Int{collectionCount(arg)};
                   }));
  builtins.emplace(LIST_FN_SYM, makeBuiltinFunction(LIST_FN_SYM, -1,
                                                    [](const ExprList &args) {
                                                      return List{args};
                                                    }));
  builtins.emplace(
      VEC_FN_SYM, makeBuiltinFunction<Expr>(VEC_FN_SYM, [](const Expr &arg) {
        if (const auto *list = std::get_if<Box<Vector>>(&arg)) {
          return arg;
        }
        if (const auto *list = std::get_if<Box<List>>(&arg)) {
          Expr vec = Vector{{(*list)->value.begin(), (*list)->value.end()}};
          return vec;
        }
        throw std::runtime_error("not a list or vector: " + prStr(arg, true));
      }));
  builtins.emplace(CONCAT_FN_SYM,
                   makeBuiltinFunction(CONCAT_FN_SYM, -1, concat));
  builtins.emplace(CONS_FN_SYM,
                   makeBuiltinFunction<Expr>(
                       CONS_FN_SYM, [](const Expr &arg1, const Expr &arg2) {
                         return concat(ExprList{List{ExprList{arg1}}, arg2});
                       }));

  // read from string/file functions
  builtins.emplace(
      "read-string",
      makeBuiltinFunction<Expr>("read-string", [](const Expr &arg) {
        if (const auto *str = std::get_if<String>(&arg)) {
          return readStr(str->value);
        }

        throw std::runtime_error("not a string: " + prStr(arg, true));
      }));
  builtins.emplace(
      "slurp", makeBuiltinFunction<Expr>("slurp", [](const Expr &arg) {
        if (const auto *str = std::get_if<String>(&arg)) {
          const std::ifstream file(str->value);
          std::stringstream buffer;
          buffer << file.rdbuf();
          return String{buffer.str()};
        }

        throw std::runtime_error("not a string: " + prStr(arg, true));
      }));

  // atom functions
  builtins.emplace("atom",
                   makeBuiltinFunction<Expr>(
                       "atom", [](const Expr &arg) { return Atom{arg}; }));
  builtins.emplace("atom?",
                   makeBuiltinFunction<bool>("atom?", [](const Expr &arg) {
                     return std::holds_alternative<Box<Atom>>(arg);
                   }));
  builtins.emplace(DEREF_FN_SYM,
                   makeBuiltinFunction<Expr>(DEREF_FN_SYM, [](const Expr &arg) {
                     if (const auto *atom = std::get_if<Box<Atom>>(&arg)) {
                       return (*atom)->value;
                     }

                     throw std::runtime_error("not an atom: " +
                                              prStr(arg, true));
                   }));
  builtins.emplace(
      "reset!", makeBuiltinFunction<Expr>("reset!", [](const Expr &arg1,
                                                       const Expr &arg2) {
        if (const auto *atom = std::get_if<Box<Atom>>(&arg1)) {
          (*atom)->value = arg2;
          return arg2;
        }

        throw std::runtime_error("not an atom: " + prStr(arg1, true));
      }));

  return builtins;
}

Expr Core::makeSwapBangFunction(std::function<Expr(const Expr &)> eval) {
  return makeBuiltinFunction(
      SWAP_BANG_FN_SYM, -1, [eval = std::move(eval)](ExprList &args) {
        checkMinArgCount(args.size(), 2);
        const auto arg = std::move(args.front());
        args.pop_front();

        if (const auto *atom = std::get_if<Box<Atom>>(&arg)) {
          args.insert(std::next(args.begin()), (*atom)->value);

          auto newValue = eval(List{args});
          (*atom)->value = newValue;
          return newValue;
        }

        throw std::runtime_error("not an atom: " + prStr(arg, true));
      });
}
