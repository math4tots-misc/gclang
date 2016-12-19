#ifndef object_hh
#define object_hh

#include "symbol.hh"
#include "error.hh"

class Object {
public:
  enum class Color { BLACK, WHITE };
  Color color = Color::WHITE;
  long refcnt = 0;

  Object()=default;
  virtual ~Object() {}

  virtual Object *get(Symbol);

};

#endif//object_hh
