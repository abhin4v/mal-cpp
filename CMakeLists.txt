cmake_minimum_required(VERSION 3.26)
set (CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)
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
add_library(mal-equator OBJECT src/equator.cpp)
add_library(mal-hasher OBJECT src/hasher.cpp)
add_library(mal-evaluator OBJECT src/evaluator.cpp)
add_library(mal-env OBJECT src/env.cpp)
add_library(mal-core OBJECT src/core.cpp)

add_executable(step0_repl src/step0_repl.cpp)
target_link_libraries(step0_repl PRIVATE ${LINENOISE_LIBRARIES})

add_executable(step1_read_print src/step1_read_print.cpp)
target_link_libraries(step1_read_print PRIVATE
  mal-tokenizer
  mal-reader
  mal-printer
  mal-types
  mal-equator
  mal-hasher
  ${LINENOISE_LIBRARIES})

add_executable(step2_eval src/step2_eval.cpp)
target_link_libraries(step2_eval PRIVATE
  mal-tokenizer
  mal-reader
  mal-printer
  mal-types
  mal-equator
  mal-hasher
  mal-env
  mal-evaluator
  mal-core
  ${LINENOISE_LIBRARIES})

add_executable(step3_env src/step3_env.cpp)
target_link_libraries(step3_env PRIVATE
  mal-tokenizer
  mal-reader
  mal-printer
  mal-types
  mal-equator
  mal-hasher
  mal-env
  mal-evaluator
  mal-core
  ${LINENOISE_LIBRARIES})

add_executable(step4_if_fn_do src/step4_if_fn_do.cpp)
target_link_libraries(step4_if_fn_do PRIVATE
  mal-tokenizer
  mal-reader
  mal-printer
  mal-types
  mal-equator
  mal-hasher
  mal-env
  mal-evaluator
  mal-core
  ${LINENOISE_LIBRARIES})

add_executable(step5_tco src/step5_tco.cpp)
target_link_libraries(step5_tco PRIVATE
  mal-tokenizer
  mal-reader
  mal-printer
  mal-types
  mal-equator
  mal-hasher
  mal-env
  mal-evaluator
  mal-core
  ${LINENOISE_LIBRARIES})

add_executable(step6_file src/step6_file.cpp)
target_link_libraries(step6_file PRIVATE
  mal-tokenizer
  mal-reader
  mal-printer
  mal-types
  mal-equator
  mal-hasher
  mal-env
  mal-evaluator
  mal-core
  ${LINENOISE_LIBRARIES})

add_executable(step7_quote src/step7_quote.cpp)
target_link_libraries(step7_quote PRIVATE
  mal-tokenizer
  mal-reader
  mal-printer
  mal-types
  mal-equator
  mal-hasher
  mal-env
  mal-evaluator
  mal-core
  ${LINENOISE_LIBRARIES})

add_executable(step8_macros src/step8_macros.cpp)
target_link_libraries(step8_macros PRIVATE
  mal-tokenizer
  mal-reader
  mal-printer
  mal-types
  mal-equator
  mal-hasher
  mal-env
  mal-evaluator
  mal-core
  ${LINENOISE_LIBRARIES})

add_executable(step9_try src/step9_try.cpp)
target_link_libraries(step9_try PRIVATE
  mal-tokenizer
  mal-reader
  mal-printer
  mal-types
  mal-equator
  mal-hasher
  mal-env
  mal-evaluator
  mal-core
  ${LINENOISE_LIBRARIES})
