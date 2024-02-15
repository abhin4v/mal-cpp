cmake_minimum_required(VERSION 3.26)

project(mal
  VERSION 1.0
  LANGUAGES CXX)

find_package(PkgConfig REQUIRED)
pkg_check_modules(READLINE REQUIRED readline)

find_package(spdlog REQUIRED)
find_package(absl REQUIRED)

add_executable(step0_repl src/step0_repl.cpp)
target_link_libraries(step0_repl PRIVATE absl::time spdlog::spdlog readline ${READLINE_LIBRARIES})
