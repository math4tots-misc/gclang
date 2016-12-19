#ifndef symbol_hh
#define symbol_hh

#include <string>

using Symbol = std::string*;

Symbol intern(const std::string&);

#endif//symbol_hh
