#include <string>
#include <vector>
#include <map>
#include <iostream>

void error(const std::string &s) {
  std::cerr << "ERROR: " << s << std::endl;
  throw s;
}

class Value;
class Object;
class Table;
class Function;
class Instruction;
class Blob;
using Int = long long;

std::string *intern(const std::string&);

class Object {
public:
  enum class Color {
    BLACK, GRAY, WHITE,
  };
  Color color = Color::WHITE;
};

class Table final: public Object {
public:
  Table *proto;
  std::map<std::string*, Value> mapping;
};

class Function final: public Object {
public:
  Table *env;
  Blob *blob;
  Function(Table *e, Blob *c): env(e), blob(c) {}
};

class Value final {
public:
  enum class Type {
    NIL, INTEGER, TABLE, FUNCTION,
  };
  Type type;
  union {
    Int i;
    Object *obj;
  };
  Value(): type(Type::NIL) {};
  Value(Type t, Int i): type(t), i(i) {}
  Value(Type t, Object *obj): type(t), obj(obj) {}
  Int integer() const { return i; }
  Table *table() const { return static_cast<Table*>(obj); }
  Function *function() const { return static_cast<Function*>(obj); }
};

std::string str(Value::Type t) {
  switch (t) {
  case Value::Type::NIL: return "NIL";
  case Value::Type::INTEGER: return "INTEGER";
  case Value::Type::TABLE: return "TABLE";
  case Value::Type::FUNCTION: return "FUNCTION";
  }
  error("Invalid Value::Type = " + std::to_string(static_cast<int>(t)));
  return "";
}

class Instruction final {
public:
  enum class Type {
    INVALID,
    PUSH_NIL,  // pushes a nil value.
    PUSH_VARIABLE,  // pushes variable value identified by 'name'
    PUSH_INTEGER,  // pushes value 'integer'
    PUSH_FUNCTION,  // pushes a function using 'blob' and current environment
    DECLARE_VARIABLE,  // declares variable using value popped from stack
    IF,  // jump to 'integer' if false (i.e. jump to else clause)
    ELSE,  // unconditional jump to 'integer' (i.e. jump to end of else)
    POP,  // pops value on top of stack -- between blocks
    CALL,  // calls function using 'integer' arguments.
    TAILCALL,  // calls function using 'integer' arguments, must be last op
    DEBUG_PRINT,  // Debugging mechanism -- print top of stack
  };
  Type type;
  union {
    Int integer;
    std::string *name;
    Blob *blob;
  };
  Instruction(Type t): type(t) {}
  Instruction(Type t, Int i): type(t), integer(i) {}
  Instruction(Type t, std::string *s): type(t), name(s) {}
  Instruction(Type t, Blob *b): type(t), blob(b) {}
};

class Blob final {
public:
  std::vector<std::string*> args;
  std::vector<Instruction> instructions;
};

class Expression final {
public:
  enum class Type {
    NIL,
    INTEGER,
    LAMBDA,
    DECLARE,
    CALL,
    IF,
    BLOCK,
    DEBUG_PRINT,
  };
  Type type;
  Int integer;
  std::vector<std::string*> names;
  std::vector<Expression> children;
  void compile(Blob&);
  Blob *compile() {
    Blob *blob = new Blob();
    compile(*blob);
    return blob;
  }
  Expression(Type t, Int i): type(t), integer(i) {}
  Expression(Type t, std::initializer_list<Expression> es): type(t), children(es) {}
};

class ProgramCounter final {
public:
  Blob *blob;
  long index;
  ProgramCounter(Blob *b, long i): blob(b), index(i) {}
  bool done() const { return static_cast<unsigned long>(index) >= blob->instructions.size(); }
  void incr() { index++; }
  Instruction &get() { return blob->instructions[index]; }
};

class VirtualMachine final {
public:
  std::vector<Value> evalstack;
  std::vector<ProgramCounter> retstack;
  std::vector<Table*> envstack;
  ProgramCounter pc;
  VirtualMachine(const ProgramCounter &p): pc(p) {}
  void run();
};

std::map<std::string, std::string*> internTable;

std::string *intern(const std::string &s) {
  if (internTable.find(s) == internTable.end()) {
    internTable[s] = new std::string(s);
  }
  return internTable[s];
}

void Expression::compile(Blob &b) {
  switch (type) {
  case Type::NIL:
    b.instructions.push_back(Instruction(Instruction::Type::PUSH_NIL));
    break;
  case Type::INTEGER:
    b.instructions.push_back(Instruction(Instruction::Type::PUSH_INTEGER, integer));
    break;
  case Type::LAMBDA: {
    Blob *blob = new Blob;
    blob->args = names;
    children[0].compile(*blob);
    b.instructions.push_back(Instruction(Instruction::Type::PUSH_FUNCTION, blob));
    break;
  }
  case Type::DECLARE:
    b.instructions.push_back(Instruction(Instruction::Type::DECLARE_VARIABLE, names[0]));
    break;
  case Type::CALL: {
    children[0].compile(b);
    for (unsigned int i = 1; i < children.size(); i++) {
      children[i].compile(b);
    }
    b.instructions.push_back(Instruction(Instruction::Type::CALL, children.size()-1));
    break;
  }
  case Type::DEBUG_PRINT:
    children[0].compile(b);
    b.instructions.push_back(Instruction(Instruction::Type::DEBUG_PRINT));
    break;
  case Type::BLOCK:
    if (children.empty()) {
      b.instructions.push_back(Instruction(Instruction::Type::PUSH_NIL));
    } else {
      for (unsigned long i = 0; i < children.size() - 1; i++) {
        children[i].compile(b);
        b.instructions.push_back(Instruction(Instruction::Type::POP));
      }
      children.back().compile(b);
    }
    break;
  case Type::IF:  // TODO
    error("NOT IMPLEMENTED");
    break;
  }
}

void VirtualMachine::run() {
  std::cout << "VirtualMachine::run" << std::endl;
  while (!(retstack.empty() && pc.done())) {
    if (pc.done()) {
      pc = retstack.back();
      retstack.pop_back();
    } else {
      Instruction &i = pc.get();
      switch (i.type) {
      case Instruction::Type::PUSH_NIL:
        evalstack.push_back(Value());
        pc.incr();
        break;
      case Instruction::Type::DEBUG_PRINT:
        std::cout << str(evalstack.back().type);
        switch (evalstack.back().type) {
        case Value::Type::INTEGER:
          std::cout << "(" << evalstack.back().integer() << ")";
          break;
        default:
          break;
        }
        std::cout << std::endl;
        pc.incr();
        break;
      case Instruction::Type::PUSH_INTEGER:
        evalstack.push_back(Value(Value::Type::INTEGER, i.integer));
        pc.incr();
        break;
      case Instruction::Type::POP:
        evalstack.pop_back();
        pc.incr();
        break;
      // TODO: ---
      case Instruction::Type::INVALID:
        error("Invalid instruction");
        break;
      case Instruction::Type::PUSH_VARIABLE:
      case Instruction::Type::PUSH_FUNCTION:
      case Instruction::Type::DECLARE_VARIABLE:
      case Instruction::Type::IF:
      case Instruction::Type::ELSE:
      case Instruction::Type::CALL:
      case Instruction::Type::TAILCALL:
        error("Not yet implemented");
      }
    }
  }
}

int main() {
  auto e = Expression(Expression::Type::BLOCK, {
    Expression(Expression::Type::DEBUG_PRINT, {
      Expression(Expression::Type::INTEGER, 124124)
    }),
    Expression(Expression::Type::DEBUG_PRINT, {
      Expression(Expression::Type::INTEGER, 7)
    })
  });
  VirtualMachine vm(ProgramCounter(e.compile(), 0));
  vm.run();
  std::cout << "hi" << std::endl;
}
