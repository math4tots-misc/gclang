#include "symbol.hh"

#include <map>

namespace {
std::map<std::string, Symbol> internTable;
}  // namespace

Symbol intern(const std::string &s) {
  auto iter = internTable.find(s);
  if (iter != internTable.end()) {
    return iter->second;
  } else {
    auto sym = new std::string(s);
    internTable[s] = sym;
    return sym;
  }
}
