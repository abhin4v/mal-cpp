BUILD_DIR := "build"
ROOT_DIR := justfile_directory()
LINENOISE_LIB_PATH := `pkg-config --libs-only-L linenoise | cut -c 3-`

_default:
  @just help

# prints the help message
help:
  @just --list --unsorted --list-heading $'Available commands:\n'

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

_build target *flags:
  cmake --build {{BUILD_DIR}} --target {{target}} {{flags}} --parallel

# builds a target
build target *flags: (configure "Debug") (_build target flags)

build-release target *flags: (configure "Release") (_build target flags)

# cleans the build
clean:
  cmake --build {{BUILD_DIR}} --target clean
  rm -f result

_run target *args:
  DYLD_LIBRARY_PATH={{LINENOISE_LIB_PATH}} {{BUILD_DIR}}/{{target}} {{args}}

# runs a target
run target *args: (build target) (_run target args)

# tests a target
test target:
  cd ../.. && make test^cpp^{{replace_regex(target, '_.+', '')}}

@lint:
  echo "============================= FLAWFINDER =============================="
  flawfinder -m0 src/*
  echo;

  echo "=============================  CPPCHECK  ============================="
  cppcheck --enable=all --disable=missingInclude --inconclusive src/*

  echo "============================= CLANG-TIDY ============================="
  clang-tidy -p compile_commands.json src/*
