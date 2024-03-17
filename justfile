BUILD_DIR := "build"
ROOT_DIR := justfile_directory()
IN_NIX_SHELL := env_var_or_default("IN_NIX_SHELL", "false")

_default:
  @just help

# prints the help message
help:
  @just --list --unsorted --list-heading $'Available commands:\n'

_run-in-nix-shell cmd *args:
    #!/usr/bin/env -S bash
    if [ "{{IN_NIX_SHELL}}" = "false" ]; then
        nix develop {{ROOT_DIR}}#minimal --command sh -c "just {{ROOT_DIR}}/{{cmd}} {{args}}"
    else
        just {{cmd}} {{args}}
    fi

# sets up the project
configure build-type:
  #!/usr/bin/env bash
  set -euo pipefail
  if [ -d "{{BUILD_DIR}}" ]; then
    BUILD_TYPE=`cmake -L -B {{BUILD_DIR}} 2>/dev/null | grep "CMAKE_BUILD_TYPE:STRING=" | cut -d '=' -f 2`
    if [ "${BUILD_TYPE}" != "{{build-type}}" ]; then
      rm -rf {{BUILD_DIR}}
    fi
  fi

  if [ ! -d "{{BUILD_DIR}}" ]; then
    echo "Configuring"
    mkdir -p {{BUILD_DIR}}
    CMAKE_BUILD_TYPE={{build-type}} cmake -B {{BUILD_DIR}} -S {{ROOT_DIR}}
  fi

# generates compile_commands.json
gen-mini-compile-commands:
  #!/usr/bin/env bash
  set -euxo pipefail
  rm -f compile_commands.json
  mini_compile_commands_server.py &
  pid=$!
  just clean
  just build all
  kill -s SIGINT $pid

[macos]
@_fixup target:
  #!/usr/bin/env bash
  TARGET="{{target}}"
  if [ "$TARGET" = "all" ]; then
    find {{BUILD_DIR}} -maxdepth 1 -name 'step*' -type f -print | cut -d '/' -f 2 | xargs -I {} just --quiet _fixup "{}"
  elif [[ ${TARGET:0:4} == "step" ]]; then
    LINENOISE_LIB_PATH=`pkg-config --libs-only-L linenoise | cut -c 3-`
    install_name_tool -change liblinenoise.so "${LINENOISE_LIB_PATH}/liblinenoise.so" {{BUILD_DIR}}/$TARGET
  fi

[linux]
@_fixup target:
  true

_build build-type target *flags: (configure build-type)
  cmake --build {{BUILD_DIR}} --target {{target}} {{flags}} --parallel
  just --quiet _fixup {{target}}

# builds a target
build target *flags: (_run-in-nix-shell "_build" "Debug" target flags)

# builds a target in release mode
build-release target *flags: (_run-in-nix-shell "_build" "Release" target flags)

_clean:
  cmake --build {{BUILD_DIR}} --target clean

# cleans the build
clean: (_run-in-nix-shell "_clean")

_run target *args:
  {{BUILD_DIR}}/{{target}} {{args}}

# runs a target
run target *args: (build target) (_run target args)

_test target:
  cd ../.. && make test^cpp^{{replace_regex(target, '_.+', '')}}

# tests a target
test target: (_test target)

_test-self-hosted target:
  cd ../.. && make MAL_IMPL=cpp test^mal^{{replace_regex(target, '_.+', '')}}

# test a target on self-hosted interpreter
test-self-hosted target: (build "stepA_mal") (_test-self-hosted target)

# tests all targets
test-all: (build "all")
  cd ../.. && make test^cpp

# tests all targets on self-hosted interpreter
test-self-hosted-all: (build "stepA_mal")
  cd ../.. && make MAL_IMPL=cpp test^mal

# tests everything
test-everything: test-all test-self-hosted-all

# runs performance benchmarks
perf: (build-release "stepA_mal")
  cd ../.. && make perf^cpp

# runs linters
@lint:
  echo "============================= FLAWFINDER =============================="
  flawfinder -m0 src/*
  echo;

  echo "=============================  CPPCHECK  ============================="
  cppcheck --enable=all --disable=missingInclude --inconclusive --inline-suppr src/*

  echo "============================= CLANG-TIDY ============================="
  clang-tidy -p compile_commands.json src/*
