# Make-A-Lisp in C++ 20

This is an implementation of [Make-A-Lisp](https://github.com/kanaka/mal) (MAL) in C++20. It uses modern C++ constructs, and eschews things like preprocessor macros, pointer arithmetics and type casting.

It has two dependencies:

- The [Immer](https://github.com/arximboldi/immer) library of immutable data structures in C++ is used as the basis of the interpreter's AST data structures. This allows us to implement MAL's immutable data structures with ease.
- The [Linenoise](https://github.com/antirez/linenoise) library is used to implement the MAL REPL with line editing and history support.

The dependencies are configured using [Nixpkgs](https://github.com/NixOS/nixpkgs/), the build is done using [CMake](https://cmake.org/), and the commands are run using [Just](https://github.com/casey/just).

## Usage

First install Nix. Then in the root directory, run: `nix develop .#minimal`. This will set up all the required dependencies and tools.

Now, build the project by running `just build all`. A particular step can also be built by running `just build <stepname>`, for example: `just build step0_repl`.

A particular step's REPL can be started by running: `just run <stepname>`, for example: `just run stepA_mal`.

## Testing

This implementation passes all tests of the [MAL](https://github.com/kanaka/mal) project. To run the tests:

1. Clone the MAL repo:

   ```
   git clone --depth 1 https://github.com/kanaka/mal
   ```

2. Replace the existing C++ implementation with this repo:

   ```
   rm -rf impls/cpp
   git clone --depth 1 https://github.com/abhin4v/mal-cpp impls/cpp
   ```

3. Install Nix, Make and Just. Then run tests from the MAL repo root:

   ```
   make test^cpp # C++ impl tests
   make MAL_IMPL=CPP test^mal # self-hosting tests
   ```

   Alternatively, install Nix (but not Make or Just), and run:

   ```
   nix develop ./impls/cpp#minimal --command sh -c "just test-everything"
   ```

`make clean^cpp` and `make perf^cpp` commands are also supported.

I have tested this implementation on MacOS 14 with Clang 16.0 and NixOS 23.11 with GCC 13.2, both built by CMake 3.27.
