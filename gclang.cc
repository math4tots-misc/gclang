#include <memory>

namespace gclang {

class Object;

class Value final {
public:
  enum class Type {
    NIL, NUMBER, STRING, ARRAY, TABLE, FUNCTION,
  };
  union {
    double number;
    Object *object;
  };
};

class Object {
public:
  virtual ~Object();
};

class String final: public Object {
public:
  const std::string buffer;
  String(const std::string &s): buffer(s) {}
};

class Expression: public std::enable_shared_from_this<Expression> {
  virtual Value eval(Value env)=0;
};

class If: public Expression {
  Value eval(Value env) override {
    return condition->eval(env).truthy() ? body->eval(env) : other->eval(env);
  }
};

}  // namespace gclang

// impl
namespace gclang {



}  // namespace gclang

int main() {}
