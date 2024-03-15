#include "core.hpp"
#include "constants.hpp"
#include "env.hpp"
#include "errors.hpp"
#include "evaluator.hpp"
#include "printer.hpp"
#include "types.hpp"

namespace {
template <typename T> bool isOfType(const Expr &arg) {
  return std::holds_alternative<T>(arg);
}

// NOLINTBEGIN
template <size_t N> struct StringLiteral {
  constexpr StringLiteral(const char (&str)[N]) { std::copy_n(str, N, value); }
  char value[N];
};
// NOLINTEND

template <typename T, StringLiteral name>
Expr makeExprVariantFunc(const std::string &funcName,
                         UnaryFunction<T, Expr> func) {
  return makeBuiltinFunction<Expr>(
      funcName, [func = std::move(func)](const Expr &arg) {
        if (const auto *var = std::get_if<T>(&arg)) {
          return func(*var);
        }

        throw WrongTypeError(static_cast<const char *>(name.value), arg);
      });
}

template <typename T, StringLiteral name>
Expr makeExprVariantFunc(const std::string &funcName,
                         std::function<Expr(const T &, const Expr &)> func) {
  return makeBuiltinFunction<Expr>(
      funcName, [func = std::move(func)](const Expr &arg1, const Expr &arg2) {
        if (const auto *var = std::get_if<T>(&arg1)) {
          return func(*var, arg2);
        }

        throw WrongTypeError(static_cast<const char *>(name.value), arg1);
      });
}
} // namespace

Builtins Core::makeBuiltins() noexcept {
  Builtins builtins;
  makeArithmeticFunctions(builtins);
  makeStringFunctions(builtins);
  makePrintFunctions(builtins);
  makeCollectionFunctions(builtins);
  makeReadFunctions(builtins);
  makeAtomFunctions(builtins);
  makeTypePredicateFunctions(builtins);

  builtins.emplace("symbol", makeExprVariantFunc<String, "a string">(
                                 "symbol", [](const auto &str) {
                                   return Symbol{str.value};
                                 }));
  builtins.emplace("keyword",
                   makeBuiltinFunction<Expr>("keyword", [](const Expr &arg) {
                     if (isOfType<Keyword>(arg)) {
                       return arg;
                     }
                     if (const auto *str = std::get_if<String>(&arg)) {
                       return Expr{Keyword{str->value}};
                     }
                     throw WrongTypeError("a string or keyword", arg);
                   }));
  builtins.emplace(APPLY_FN_SYM,
                   makeBuiltinFunction(APPLY_FN_SYM, -1, Core::apply));
  builtins.emplace("throw",
                   makeBuiltinFunction<Expr>("throw", [](const Expr &arg) {
                     throw MALException(arg);
                     return Expr{Nil::NIL};
                   }));
  builtins.emplace("readline", makeExprVariantFunc<String, "a string">(
                                   "readline", Core::readline));
  builtins.emplace(
      "time-ms",
      makeBuiltinFunction("time-ms", 0, [](const ExprList & /*args*/) {
        namespace chrono = std::chrono;
        return Int{chrono::duration_cast<chrono::milliseconds>(
                       chrono::system_clock::now().time_since_epoch())
                       .count()};
      }));
  builtins.emplace("meta", makeBuiltinFunction<Expr>("meta", Core::meta));
  return builtins;
}

void Core::makeArithmeticFunctions(Builtins &builtins) {
  builtins.emplace(
      "+", makeBuiltinFunction("+", [](const std::vector<std::int64_t> &args) {
        return std::accumulate(args.begin(), args.end(), 0LL);
      }));
  builtins.emplace(
      "*", makeBuiltinFunction("*", [](const std::vector<std::int64_t> &args) {
        return std::accumulate(args.begin(), args.end(), 1LL,
                               std::multiplies<>());
      }));
  builtins.emplace("-", makeBuiltinFunction<std::int64_t>("-", std::minus<>()));
  builtins.emplace("/",
                   makeBuiltinFunction<std::int64_t>("/", std::divides<>()));

  // NOLINTBEGIN(modernize-use-transparent-functors)
  builtins.emplace("<",
                   makeBuiltinFunction<bool>("<", std::less<std::int64_t>()));
  builtins.emplace(
      ">", makeBuiltinFunction<bool>(">", std::greater<std::int64_t>()));
  builtins.emplace(
      "<=", makeBuiltinFunction<bool>("<=", std::less_equal<std::int64_t>()));
  builtins.emplace(">=", makeBuiltinFunction<bool>(
                             ">=", std::greater_equal<std::int64_t>()));
  // NOLINTEND(modernize-use-transparent-functors)

  builtins.emplace("=", makeBuiltinFunction<bool>("=", exprEquals));
}

void Core::makeStringFunctions(Builtins &builtins) {
  builtins.emplace("pr-str",
                   makeBuiltinFunction("pr-str", -1, [](const ExprList &args) {
                     return String{toString(args, true, " ")};
                   }));
  builtins.emplace("str",
                   makeBuiltinFunction("str", -1, [](const ExprList &args) {
                     return String{toString(args, false, "")};
                   }));
}

void Core::makePrintFunctions(Builtins &builtins) {
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
}

void Core::makeCollectionFunctions(Builtins &builtins) {
  builtins.emplace(LIST_FN_SYM, makeBuiltinFunction(LIST_FN_SYM, -1,
                                                    [](const ExprList &args) {
                                                      return List{args};
                                                    }));
  builtins.emplace("vector",
                   makeBuiltinFunction("vector", -1, [](const ExprList &args) {
                     return Vector{{args.begin(), args.end()}};
                   }));
  builtins.emplace("hash-map",
                   makeBuiltinFunction("hash-map", -1, makeHashMap));

  builtins.emplace("map", makeBuiltinFunction<Expr>("map", map));
  builtins.emplace("empty?",
                   makeBuiltinFunction<bool>("empty?", [](const Expr &arg) {
                     return collectionCount(arg) == 0;
                   }));
  builtins.emplace("count",
                   makeBuiltinFunction<Expr>("count", [](const Expr &arg) {
                     return Int{collectionCount(arg)};
                   }));
  builtins.emplace(
      VEC_FN_SYM, makeBuiltinFunction<Expr>(VEC_FN_SYM, [](const Expr &arg) {
        if (const auto *vec = std::get_if<Box<Vector>>(&arg)) {
          return arg;
        }
        if (const auto *list = std::get_if<Box<List>>(&arg)) {
          Expr vec = Vector{{(*list)->value.begin(), (*list)->value.end()}};
          return vec;
        }
        throw WrongTypeError("a list or vector", arg);
      }));
  builtins.emplace("seq", makeBuiltinFunction<Expr>("seq", Core::seq));
  builtins.emplace("conj", makeBuiltinFunction("conj", -1, Core::conj));
  builtins.emplace(CONCAT_FN_SYM,
                   makeBuiltinFunction(CONCAT_FN_SYM, -1, concat));
  builtins.emplace(CONS_FN_SYM,
                   makeBuiltinFunction<Expr>(
                       CONS_FN_SYM, [](const Expr &arg1, const Expr &arg2) {
                         return concat(ExprList{List{ExprList{arg1}}, arg2});
                       }));
  builtins.emplace("nth", makeBuiltinFunction<Expr>("nth", Core::nthElement));
  builtins.emplace("first",
                   makeBuiltinFunction<Expr>("first", Core::firstElement));
  builtins.emplace("rest",
                   makeBuiltinFunction<Expr>("rest", Core::restElements));

  builtins.emplace("assoc", makeBuiltinFunction("assoc", -1, Core::assoc));
  builtins.emplace("dissoc", makeBuiltinFunction("dissoc", -1, Core::dissoc));
  builtins.emplace("get", makeBuiltinFunction<Expr>("get", Core::get));
  builtins.emplace("contains?",
                   makeExprVariantFunc<Box<HashMap>, "a hashmap">(
                       "contains?", [](const auto &hashmap, const Expr &arg2) {
                         return hashmap->value.contains(arg2) ? Bool::TRUE
                                                              : Bool::FALSE;
                       }));
  builtins.emplace("keys", makeExprVariantFunc<Box<HashMap>, "a hashmap">(
                               "keys", Core::keys));
  builtins.emplace("vals", makeExprVariantFunc<Box<HashMap>, "a hashmap">(
                               "vals", Core::vals));
}

void Core::makeReadFunctions(Builtins &builtins) {
  builtins.emplace("read-string", makeExprVariantFunc<String, "a string">(
                                      "read-string", [](const auto &str) {
                                        return readStr(str.value);
                                      }));
  builtins.emplace("slurp", makeExprVariantFunc<String, "a string">(
                                "slurp", [](const auto &str) {
                                  const std::ifstream file(str.value);
                                  std::stringstream buffer;
                                  buffer << file.rdbuf();
                                  return String{buffer.str()};
                                }));
}

void Core::makeAtomFunctions(Builtins &builtins) {
  builtins.emplace("atom",
                   makeBuiltinFunction<Expr>(
                       "atom", [](const Expr &arg) { return Atom{arg}; }));
  builtins.emplace(DEREF_FN_SYM, makeExprVariantFunc<Box<Atom>, "an atom">(
                                     DEREF_FN_SYM, [](const auto &atom) {
                                       return atom->value;
                                     }));
  builtins.emplace("reset!",
                   makeExprVariantFunc<Box<Atom>, "an atom">(
                       "reset!", [](const auto &atom, const Expr &arg2) {
                         atom->value = arg2;
                         return arg2;
                       }));
}

void Core::makeTypePredicateFunctions(Builtins &builtins) {
  builtins.emplace("nil?", makeBuiltinFunction<bool>("nil?", isOfType<Nil>));
  builtins.emplace(
      "true?", makeBuiltinFunction<bool>("true?", [](const Expr &arg) {
        return isOfType<Bool>(arg) && std::get<Bool>(arg) == Bool::TRUE;
      }));
  builtins.emplace(
      "false?", makeBuiltinFunction<bool>("false?", [](const Expr &arg) {
        return isOfType<Bool>(arg) && std::get<Bool>(arg) == Bool::FALSE;
      }));
  builtins.emplace("string?",
                   makeBuiltinFunction<bool>("string?", isOfType<String>));
  builtins.emplace("number?",
                   makeBuiltinFunction<bool>("number?", isOfType<Int>));
  builtins.emplace("symbol?",
                   makeBuiltinFunction<bool>("symbol?", isOfType<Symbol>));
  builtins.emplace("keyword?",
                   makeBuiltinFunction<bool>("keyword?", isOfType<Keyword>));
  builtins.emplace("atom?",
                   makeBuiltinFunction<bool>("atom?", isOfType<Box<Atom>>));
  builtins.emplace("list?",
                   makeBuiltinFunction<bool>("list?", isOfType<Box<List>>));
  builtins.emplace("vector?",
                   makeBuiltinFunction<bool>("vector?", isOfType<Box<Vector>>));
  builtins.emplace("map?",
                   makeBuiltinFunction<bool>("map?", isOfType<Box<HashMap>>));

  builtins.emplace("fn?", makeBuiltinFunction<bool>("fn?", [](const Expr &arg) {
                     return isOfType<Box<BuiltinFunction>>(arg) ||
                            (isOfType<Box<Lambda>>(arg) &&
                             !std::get<Box<Lambda>>(arg)->macro);
                   }));
  builtins.emplace(
      "macro?", makeBuiltinFunction<bool>("macro?", [](const Expr &arg) {
        return isOfType<Box<Lambda>>(arg) && std::get<Box<Lambda>>(arg)->macro;
      }));
  builtins.emplace(
      "sequential?",
      makeBuiltinFunction<bool>("sequential?", [](const Expr &arg) {
        return isOfType<Box<List>>(arg) || isOfType<Box<Vector>>(arg);
      }));
}

Expr Core::readline(const String &str) {
  std::cout << str.value;
  std::string line;
  std::getline(std::cin, line);
  if (std::cin.eof()) {
    return Expr{Nil::NIL};
  }
  return Expr{String{line}};
}

Expr Core::apply(ExprList &args) {
  const auto argCount = args.size();
  checkMinArgCount(argCount, 2);

  auto callee = args.front();
  args.pop_front();
  auto &lastArg = args.back();

  if (const auto *list = std::get_if<Box<List>>(&lastArg)) {
    auto &elements = (*list)->value;
    for (auto iter = std::next(args.rbegin()); iter != args.rend(); ++iter) {
      elements.push_front(*iter);
    }
    return Evaluator::apply(callee, elements);
  }
  if (const auto *vector = std::get_if<Box<Vector>>(&lastArg)) {
    ExprList elements{(*vector)->value.begin(), (*vector)->value.end()};
    for (auto iter = std::next(args.rbegin()); iter != args.rend(); ++iter) {
      elements.push_front(*iter);
    }
    return Evaluator::apply(callee, elements);
  }

  throw WrongTypeError("a list", lastArg);
}

Expr Core::map(const Expr &arg1, const Expr &arg2) {
  if (const auto *vec = std::get_if<Box<Vector>>(&arg2)) {
    ExprList ret;
    for (const auto &element : (*vec)->value) {
      ExprList args{element};
      ret.push_back(Evaluator::apply(arg1, args));
    }
    return List{ret};
  }
  if (const auto *list = std::get_if<Box<List>>(&arg2)) {
    ExprList ret;
    for (const auto &element : (*list)->value) {
      ExprList args{element};
      ret.push_back(Evaluator::apply(arg1, args));
    }
    return List{ret};
  }
  throw WrongTypeError("a list or vector", arg2);
}

std::int64_t Core::collectionCount(const Expr &arg) {
  if (isOfType<Nil>(arg)) {
    return 0;
  }
  if (const auto *list = std::get_if<Box<List>>(&arg)) {
    return static_cast<std::int64_t>((*list)->value.size());
  }
  if (const auto *vector = std::get_if<Box<Vector>>(&arg)) {
    return static_cast<std::int64_t>((*vector)->value.size());
  }
  if (const auto *hashmap = std::get_if<Box<HashMap>>(&arg)) {
    return static_cast<std::int64_t>((*hashmap)->value.size());
  }
  throw WrongTypeError("a list, vector or hashmap", arg);
}

Expr Core::concat(const ExprList &args) {
  if (args.empty()) {
    return List{{}};
  }

  ExprList newList;
  for (const auto &arg : args) {
    if (const auto *list = std::get_if<Box<List>>(&arg)) {
      newList.insert(newList.end(), (*list)->value.begin(),
                     (*list)->value.end());
      continue;
    }
    if (const auto *vec = std::get_if<Box<Vector>>(&arg)) {
      newList.insert(newList.end(), (*vec)->value.begin(), (*vec)->value.end());
      continue;
    }
    throw WrongTypeError("a list or vector", arg);
  }

  return List{newList};
}

Expr Core::nthElement(const Expr &arg1, const Expr &arg2) {
  if (const auto *idxPtr = std::get_if<Int>(&arg2)) {
    auto idx = (*idxPtr).value;
    if (const auto *vec = std::get_if<Box<Vector>>(&arg1)) {
      const auto &elements = (*vec)->value;
      checkIndex(elements.size(), idx);
      return elements[idx];
    }
    if (const auto *list = std::get_if<Box<List>>(&arg1)) {
      const auto &elements = (*list)->value;
      checkIndex(elements.size(), idx);
      return *nth(elements, idx);
    }

    throw WrongTypeError("a list or vector", arg1);
  }

  throw WrongTypeError("an int", arg2);
}

Expr Core::firstElement(const Expr &arg) {
  Expr nil = Nil::NIL;
  if (isOfType<Nil>(arg)) {
    return nil;
  }

  if (const auto *vec = std::get_if<Box<Vector>>(&arg)) {
    const auto &elements = (*vec)->value;
    return elements.empty() ? nil : elements[0];
  }

  if (const auto *list = std::get_if<Box<List>>(&arg)) {
    const auto &elements = (*list)->value;
    return elements.empty() ? nil : elements.front();
  }

  throw WrongTypeError("a list or vector", arg);
}

Expr Core::restElements(const Expr &arg) {
  Expr emptyList = List{{}};
  if (isOfType<Nil>(arg)) {
    return emptyList;
  }

  if (const auto *vec = std::get_if<Box<Vector>>(&arg)) {
    const auto &elements = (*vec)->value;
    return elements.empty()
               ? emptyList
               : List{ExprList{std::next(elements.begin()), elements.end()}};
  }

  if (const auto *list = std::get_if<Box<List>>(&arg)) {
    const auto &elements = (*list)->value;
    return elements.empty()
               ? emptyList
               : List{ExprList{std::next(elements.begin()), elements.end()}};
  }

  throw WrongTypeError("a list or vector", arg);
}

Expr Core::seq(const Expr &arg) {
  if (isOfType<Nil>(arg)) {
    return arg;
  }
  if (const auto *list = std::get_if<Box<List>>(&arg)) {
    if ((*list)->value.empty()) {
      return Nil::NIL;
    }
    return arg;
  }
  if (const auto *vec = std::get_if<Box<Vector>>(&arg)) {
    if ((*vec)->value.empty()) {
      return Nil::NIL;
    }
    return List{ExprList{(*vec)->value.begin(), (*vec)->value.end()}};
  }
  if (const auto *str = std::get_if<String>(&arg)) {
    if (str->value.empty()) {
      return Nil::NIL;
    }
    ExprList list;
    std::transform(str->value.begin(), str->value.end(),
                   std::back_inserter(list),
                   [](auto chr) { return String{std::string(1, chr)}; });
    return List{list};
  }
  throw WrongTypeError("a list, vector or string", arg);
}

Expr Core::conj(ExprList &args) {
  checkMinArgCount(args.size(), 2);
  const auto coll = args.front();
  args.pop_front();

  if (const auto *vec = std::get_if<Box<Vector>>(&coll)) {
    const auto &elements = (*vec)->value;
    ExprVec newElements{elements.begin(), elements.end()};

    std::transform(args.begin(), args.end(), std::back_inserter(newElements),
                   std::identity());
    return Vector{newElements};
  }

  if (const auto *list = std::get_if<Box<List>>(&coll)) {
    const auto &elements = (*list)->value;
    ExprList newElements{elements.begin(), elements.end()};
    std::transform(args.begin(), args.end(), std::front_inserter(newElements),
                   std::identity());
    return List{newElements};
  }

  throw WrongTypeError("a list or vector", coll);
}

Expr Core::assoc(ExprList &args) {
  checkMinArgCount(args.size(), 1);

  if (const auto *hashmap = std::get_if<Box<HashMap>>(&args.front())) {
    ExprMap map{(*hashmap)->value};
    args.pop_front();
    auto assocs = makeHashMap(args);
    assocs.value.merge(map);
    return HashMap{assocs};
  }

  throw WrongTypeError("a hashmap", args.front());
}

Expr Core::dissoc(ExprList &args) {
  checkMinArgCount(args.size(), 1);

  if (const auto *hashmap = std::get_if<Box<HashMap>>(&args.front())) {
    ExprMap map{(*hashmap)->value};
    args.pop_front();
    for (const auto &key : args) {
      map.erase(key);
    }
    return HashMap{map};
  }

  throw WrongTypeError("a hashmap", args.front());
}

Expr Core::get(const Expr &arg1, const Expr &arg2) {
  if (isOfType<Nil>(arg1)) {
    return Expr{Nil::NIL};
  }

  if (const auto *hashmap = std::get_if<Box<HashMap>>(&arg1)) {
    return (*hashmap)->value.contains(arg2) ? (*hashmap)->value.at(arg2)
                                            : Expr{Nil::NIL};
  }

  throw WrongTypeError("a hashmap", arg1);
}

Expr Core::keys(const Box<HashMap> &hashmap) {
  ExprList keys;
  std::transform(hashmap->value.begin(), hashmap->value.end(),
                 std::back_inserter(keys),
                 [](const auto &entry) { return entry.first; });
  return List{keys};
}

Expr Core::vals(const Box<HashMap> &hashmap) {
  ExprList vals;
  std::transform(hashmap->value.begin(), hashmap->value.end(),
                 std::back_inserter(vals),
                 [](const auto &entry) { return entry.second; });
  return List{vals};
}
Expr Core::meta(const Expr &arg) {
  if (isOfType<Box<BuiltinFunction>>(arg)) {
    return Nil::NIL;
  }
  if (const auto *lambda = std::get_if<Box<Lambda>>(&arg)) {
    return (*lambda)->metadata;
  }
  if (const auto *list = std::get_if<Box<List>>(&arg)) {
    return (*list)->metadata;
  }
  if (const auto *vector = std::get_if<Box<Vector>>(&arg)) {
    return (*vector)->metadata;
  }
  if (const auto *hashmap = std::get_if<Box<HashMap>>(&arg)) {
    return (*hashmap)->metadata;
  }

  throw WrongTypeError("a function, list, vector or hashmap", arg);
}

Expr Core::makeWithMetaFunction(const EnvPtr &env) {
  return makeBuiltinFunction(WITH_META_FN_SYM, 2, [&](ExprList &args) {
    checkArgCount(args.size(), 2);
    const auto &arg1 = args.front();
    const auto &arg2 = args.back();

    if (const auto *funcPtr = std::get_if<Box<BuiltinFunction>>(&arg1)) {
      const auto func = funcPtr->getRaw();
      Lambda lambda{
          .params = std::vector<std::string>{{"args"}},
          .variadic = true,
          .macro = false,
          .body =
              List{{Symbol{APPLY_FN_SYM}, Symbol{func.name}, Symbol{"args"}}},
          .env = env,
          .metadata = arg2,
      };
      return Expr{lambda};
    }
    if (const auto *lambda = std::get_if<Box<Lambda>>(&arg1)) {
      auto lambdaCopy = (*lambda).getRaw();
      lambdaCopy.metadata = arg2;
      return Expr{lambdaCopy};
    }
    if (const auto *list = std::get_if<Box<List>>(&arg1)) {
      auto listCopy = (*list).getRaw();
      listCopy.metadata = arg2;
      return Expr{listCopy};
    }
    if (const auto *vector = std::get_if<Box<Vector>>(&arg1)) {
      auto vectorCopy = (*vector).getRaw();
      vectorCopy.metadata = arg2;
      return Expr{vectorCopy};
    }
    if (const auto *hashmap = std::get_if<Box<HashMap>>(&arg1)) {
      auto hashmapCopy = (*hashmap).getRaw();
      hashmapCopy.metadata = arg2;
      return Expr{hashmapCopy};
    }

    throw WrongTypeError("a function, list, vector or hashmap", arg1);
  });
}

Expr Core::makeSwapBangFunction(std::function<Expr(const Expr &)> eval) {
  return makeBuiltinFunction(
      SWAP_BANG_FN_SYM, -1, [eval = std::move(eval)](ExprList &args) {
        checkMinArgCount(args.size(), 2);
        const auto arg = std::move(args.front());
        args.pop_front();

        if (const auto *atom = std::get_if<Box<Atom>>(&arg)) {
          const auto callee = std::move(args.front());
          args.front() = (*atom)->value;

          auto newValue = Evaluator::apply(callee, args);
          (*atom)->value = newValue;
          return newValue;
        }

        throw WrongTypeError("an atom", arg);
      });
}
