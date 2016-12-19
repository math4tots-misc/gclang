#include "error.hh"

[[ noreturn ]] void error(const std::string &message) {
  throw message;
}
