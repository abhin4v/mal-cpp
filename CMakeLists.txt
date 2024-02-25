cmake_minimum_required(VERSION 3.26)
set (CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
if(DEFINED ENV{SANITIZE})
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address,leak,undefined")
endif()

project(mal
  VERSION 1.0
  LANGUAGES CXX)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LINENOISE REQUIRED linenoise)

find_package(absl REQUIRED)

add_library(mal-tokenizer OBJECT src/tokenizer.cpp)
add_library(mal-reader OBJECT src/reader.cpp)
add_library(mal-printer OBJECT src/printer.cpp)
add_library(mal-types OBJECT src/types.cpp)
add_library(mal-evaluator OBJECT src/evaluator.cpp)
add_library(mal-env OBJECT src/env.cpp)

add_executable(step0_repl src/step0_repl.cpp)
target_link_libraries(step0_repl PRIVATE ${LINENOISE_LIBRARIES})

add_executable(step1_read_print src/step1_read_print.cpp)
target_link_libraries(step1_read_print PRIVATE
  mal-tokenizer
  mal-reader
  mal-printer
  mal-types
  ${LINENOISE_LIBRARIES})

add_executable(step2_eval src/step2_eval.cpp)
target_link_libraries(step2_eval PRIVATE
  mal-tokenizer
  mal-reader
  mal-printer
  mal-types
  mal-env
  mal-evaluator
  ${LINENOISE_LIBRARIES})
