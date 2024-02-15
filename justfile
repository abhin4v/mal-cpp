BUILD_DIR := "build"
ROOT_DIR := justfile_directory()

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
  just build
  kill -s SIGINT $pid

_build target *flags:
  cmake --build {{BUILD_DIR}} --target {{target}} {{flags}}

# builds the project
build target *flags: (configure "Debug") (_build target flags)

build-release target *flags: (configure "Release") (_build target flags)

# cleans the build
clean:
  cmake --build {{BUILD_DIR}} --target clean
  rm -f result

# runs the project
run target: (build target)
  {{BUILD_DIR}}/{{target}}

test target:
  cd ../.. && make test^cpp^{{replace_regex(target, '_.+', '')}}
