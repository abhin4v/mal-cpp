cmake_minimum_required(VERSION 3.26)
set (CMAKE_CXX_STANDARD 20)
if(DEFINED ENV{SANITIZE})
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address,leak,undefined")
endif()

project(mal
  VERSION 1.0
  LANGUAGES CXX)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LINENOISE REQUIRED linenoise)

find_package(spdlog REQUIRED)
find_package(absl REQUIRED)

add_executable(step0_repl src/step0_repl.cpp)
target_link_libraries(step0_repl PRIVATE absl::time spdlog::spdlog ${LINENOISE_LIBRARIES})

add_executable(step1_read_print
  src/step1_read_print.cpp
  src/tokenizer.cpp
  src/reader.cpp
  src/printer.cpp
  src/types.cpp)
target_link_libraries(step1_read_print PRIVATE absl::time spdlog::spdlog ${LINENOISE_LIBRARIES})
