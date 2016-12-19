#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <sstream>

#define DEBUG_GC 1

namespace gclang {

class Object;
class Expression;

using P = Object*;
using E = std::shared_ptr<Expression>;

std::vector<P> allManagedObjects;
long threshold = 1000;

void markAndSweep();

template <class T, class ...Args>
T *make(Args &&...args) {

#if DEBUG_GC
  // NOTE: For debugging, do a full markAndSweep every time we allocate an
  // object
  markAndSweep();
#else
  if (allManagedObjects.size() > threshold) {
    markAndSweep();
  }
#endif

  T *t = new T(std::forward<Args>(args)...);
  allManagedObjects.push_back(t);
  return t;
}

class Object {
public:
  enum class Color { BLACK, WHITE };
  Color color = Color::WHITE;
  long refcnt = 0;

  Object()=default;
  virtual ~Object() {}
  virtual void traverse(std::function<void(P)>)=0;
  virtual bool truthy() { return true; }
  virtual bool equals(P p) { return this == p; }
  virtual std::string debugstr() const {
    std::stringstream ss;
    ss << typeid(*this).name() << "@" << this;
    return ss.str();
  }
};

class StackPointer final {
private:
  const P p;
public:
  StackPointer(P ptr): p(ptr) { p->refcnt++; }
  ~StackPointer() { p->refcnt--; }
  P get() const { return p; }
  P operator->() const { return get(); }
  operator P() const { return get(); }
};

class Nil final: public Object {
  void traverse(std::function<void(P)>) override {}
  bool truthy() override { return false; }
  std::string debugstr() const override { return "nil"; }
};

StackPointer nil(new Nil());

class Number final: public Object {
public:
  const double value;
  Number(double v): value(v) {}
  bool truthy() override { return value != 0; }
  void traverse(std::function<void(P)>) override {}
  bool equals(P p) override {
    if (this == p) { return true; }
    auto q = dynamic_cast<Number*>(p);
    return q && value == q->value;
  }
  std::string debugstr() const override {
    std::stringstream ss;
    ss << "num(" << value << ")";
    return ss.str();
  }
};

P mkn(double d) { return make<Number>(d); }

class String final: public Object {
public:
  const std::string buffer;
  String(const std::string &s): buffer(s) {}
  void traverse(std::function<void(P)>) override {}
  bool truthy() override { return !buffer.empty(); }
  bool equals(P p) override {
    if (this == p) { return true; }
    auto q = dynamic_cast<String*>(p);
    return q && buffer == q->buffer;
  }
};

P mks(const std::string &s) { return make<String>(s); }

class Array final: public Object {
public:
  std::vector<P> buffer;
  Array(const std::vector<P> &v): buffer(v) {}
  void traverse(std::function<void(P)> f) override {
    for (P p: buffer) {
      f(p);
    }
  }
  bool equals(P p) override {
    if (this == p) { return true; }
    auto q = dynamic_cast<Array*>(p);
    if (buffer.size() != q->buffer.size()) {
      return false;
    }
    for (unsigned long i = 0; i < buffer.size(); i++) {
      if (!buffer[i]->equals(q->buffer[i])) {
        return false;
      }
    }
    return true;
  }
};

class Expression: public std::enable_shared_from_this<Expression> {
public:
  virtual P eval(P env)=0;
};

class Literal: public Expression {
public:
  StackPointer value;
  Literal(P v): value(v) {}
  P eval(P) override { return value.get(); }
};

E mklit(P v) { return std::make_shared<Literal>(v); }

class If: public Expression {
public:
  E condition, body, other;
  If(E c, E b, E t): condition(c), body(b), other(t) {}
  P eval(P env) override {
    if (condition->eval(env)->truthy())
      return body->eval(env);
    else
      return other->eval(env);
  }
};

E mkif(E c, E b, E t) { return std::make_shared<If>(c, b, t); }

class Block: public Expression {
public:
  std::vector<E> statements;
  Block(std::vector<E> stmts): statements(stmts) {}
  P eval(P env) {
    if (statements.empty()) {
      return nil;
    } else {
      for (unsigned int i = 0; i < statements.size(); i++) {
        statements[i]->eval(env);
      }
      return statements.back()->eval(env);
    }
  }
};

E mkblock(std::vector<E> stmts) { return std::make_shared<Block>(stmts); }

void markAndSweep() {
  long workDone = 0;
  // mark
  std::vector<P> greyStack;
  for (P p: allManagedObjects) {
    workDone++;
    if (p->refcnt > 0 && p->color == Object::Color::WHITE) {
      p->color = Object::Color::BLACK;
      greyStack.push_back(p);
    }
  }
  while (!greyStack.empty()) {
    P p = greyStack.back();
    greyStack.pop_back();
    p->traverse([&](P q) {
      workDone++;
      if (q->color == Object::Color::WHITE) {
        q->color = Object::Color::BLACK;
        greyStack.push_back(q);
      }
    });
  }
  // sweep
  std::vector<P> survivors;
  for (P p: allManagedObjects) {
    if (p->color == Object::Color::WHITE) {
      delete p;
    } else {
      p->color = Object::Color::WHITE;
      survivors.push_back(p);
    }
  }
  threshold = workDone * 3 + 1000;

  allManagedObjects = std::move(survivors);
}

}  // namespace gclang

int main() {
  using namespace gclang;
  auto b = mkblock({
    mklit(mkn(5)),
  });
  auto r = b->eval(nullptr);
  std::cout << r->equals(mkn(5)) << std::endl;
  std::cout << r->equals(mks("Hello world!")) << std::endl;
  std::cout << mks("Hello world!")->equals(mks("Hello world!")) << std::endl;
  auto c = mkif(mklit(mkn(0)), mklit(nil), mklit(mkn(5)))->eval(nullptr);
  std::cout << c->debugstr() << std::endl;
}
