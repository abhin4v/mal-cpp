#pragma once

#include <stdexcept>

class MALError : public std::runtime_error {
public:
  explicit MALError(const std::string &message) : std::runtime_error(message) {}
};
