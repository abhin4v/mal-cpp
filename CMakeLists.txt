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

find_package(Immer REQUIRED)

add_library(mal-tokenizer OBJECT src/tokenizer.cpp)
add_library(mal-reader OBJECT src/reader.cpp)
add_library(mal-types OBJECT src/types.cpp)
add_library(mal-evaluator OBJECT src/evaluator.cpp)
add_library(mal-core OBJECT src/core.cpp)

set(MAL_LIBS mal-tokenizer mal-reader mal-types mal-evaluator mal-core ${LINENOISE_LIBRARIES})

add_executable(step0_repl src/step0_repl.cpp)
add_executable(step1_read_print src/step1_read_print.cpp)
add_executable(step2_eval src/step2_eval.cpp)
add_executable(step3_env src/step3_env.cpp)
add_executable(step4_if_fn_do src/step4_if_fn_do.cpp)
add_executable(step5_tco src/step5_tco.cpp)
add_executable(step6_file src/step6_file.cpp)
add_executable(step7_quote src/step7_quote.cpp)
add_executable(step8_macros src/step8_macros.cpp)
add_executable(step9_try src/step9_try.cpp)
add_executable(stepA_mal src/stepA_mal.cpp)

set(MAL_EXES
  step0_repl
  step1_read_print
  step2_eval
  step3_env
  step4_if_fn_do
  step5_tco
  step6_file
  step7_quote
  step8_macros
  step9_try
  stepA_mal)

foreach(EXE ${MAL_EXES})
  target_link_libraries(${EXE} PRIVATE ${MAL_LIBS})
  if(APPLE)
    add_custom_command(TARGET ${EXE} POST_BUILD
        COMMAND install_name_tool -change liblinenoise.so ${LINENOISE_LIBRARY_DIRS}/liblinenoise.so $<TARGET_FILE:${EXE}>
    )
  endif()
endforeach()

install(TARGETS ${MAL_EXES})
