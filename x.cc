#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <sstream>
#include <iomanip>

std::string error(const std::string &s) {
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
  Table(): proto(nullptr) {}
  Table(Table *p): proto(p) {}
  Value get(std::string *key) const;
  void declare(std::string *key, Value value);
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
  bool truthy() const { return type != Type::NIL; }
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
    BLOCK_START,  // pushes a new env on the envstack
    BLOCK_END,  // pops envstack
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
  std::string str();
};

std::string str(Instruction::Type t) {
  switch (t) {
  case Instruction::Type::INVALID: return "INVALID";
  case Instruction::Type::PUSH_NIL: return "PUSH_NIL";
  case Instruction::Type::PUSH_VARIABLE: return "PUSH_VARIABLE";
  case Instruction::Type::PUSH_INTEGER: return "PUSH_INTEGER";
  case Instruction::Type::PUSH_FUNCTION: return "PUSH_FUNCTION";
  case Instruction::Type::DECLARE_VARIABLE: return "DECLARE_VARIABLE";
  case Instruction::Type::BLOCK_START: return "BLOCK_START";
  case Instruction::Type::BLOCK_END: return "BLOCK_END";
  case Instruction::Type::IF: return "IF";
  case Instruction::Type::ELSE: return "ELSE";
  case Instruction::Type::POP: return "POP";
  case Instruction::Type::CALL: return "CALL";
  case Instruction::Type::TAILCALL: return "TAILCALL";
  case Instruction::Type::DEBUG_PRINT: return "DEBUG_PRINT";
  }
  error("Invalid Instruction::Type = " + std::to_string(static_cast<int>(t)));
  return "";
}

class Blob final {
public:
  std::vector<std::string*> args;
  std::vector<Instruction> instructions;
  std::string headers() {
    std::ostringstream ss;
    ss << "nargs = " << args.size();
    for (auto arg: args) {
      ss << " " << (*arg);
    }
    return ss.str();
  }
  std::string str() {
    std::ostringstream ss;
    ss << std::left;
    ss << headers();
    ss << std::endl;
    for (unsigned int i = 0; i < instructions.size(); i++) {
      ss << std::setw(7) << i << " ";
      ss << std::setw(18) << ::str(instructions[i].type);
      switch (instructions[i].type) {
      case Instruction::Type::IF:
      case Instruction::Type::ELSE:
      case Instruction::Type::CALL:
      case Instruction::Type::PUSH_INTEGER:
        ss << instructions[i].integer;
        break;
      case Instruction::Type::DECLARE_VARIABLE:
      case Instruction::Type::PUSH_VARIABLE:
        ss << (*instructions[i].name);
        break;
      case Instruction::Type::PUSH_FUNCTION:
        ss << ":";
        for (auto arg: instructions[i].blob->args) {
          ss << " " << (*arg);
        }
        break;
      default:
        break;
      }
      ss << std::endl;
    }
    return ss.str();
  }
};

class Expression final {
public:
  enum class Type {
    NIL,  // nil
    INTEGER,  // integer
    VARIABLE,  // variable
    LAMBDA,  // TODO
    DECLARE,  // names[0] -> eval(children[0])
    CALL,  // apply(eval(children[0]), map(eval, children[1...])
    IF,  // eval(children[0]) ? eval(children[1]) : eval(children[2])
    BLOCK,  // eval(children[-1])
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
  Expression()=default;
};

std::string str(Expression::Type t) {
  switch (t) {
  case Expression::Type::NIL: return "NIL";
  case Expression::Type::INTEGER: return "INTEGER";
  case Expression::Type::VARIABLE: return "VARIABLE";
  case Expression::Type::LAMBDA: return "LAMBDA";
  case Expression::Type::DECLARE: return "DECLARE";
  case Expression::Type::CALL: return "CALL";
  case Expression::Type::IF: return "IF";
  case Expression::Type::BLOCK: return "BLOCK";
  case Expression::Type::DEBUG_PRINT: return "DEBUG_PRINT";
  }
  error("Invalid Expression::Type = " + std::to_string(static_cast<int>(t)));
  return "";
}

Expression nilexpr() {
  Expression e;
  e.type = Expression::Type::NIL;
  return e;
}

Expression intexpr(Int i) {
  Expression e;
  e.type = Expression::Type::INTEGER;
  e.integer = i;
  return e;
}

Expression funcexpr(std::vector<std::string*> args, Expression body) {
  Expression e;
  e.type = Expression::Type::LAMBDA;
  e.names.assign(args.begin(), args.end());
  e.children.push_back(body);
  return e;
}

Expression declexpr(std::string *s, Expression v) {
  Expression e;
  e.type = Expression::Type::DECLARE;
  e.names.push_back(s);
  e.children.push_back(v);
  return e;
}

Expression callexpr(Expression f, std::vector<Expression> args) {
  Expression e;
  e.type = Expression::Type::CALL;
  e.children.push_back(f);
  for (auto &arg: args) {
    e.children.push_back(arg);
  }
  return e;
}

Expression varexpr(std::string *s) {
  Expression e;
  e.type = Expression::Type::VARIABLE;
  e.names.push_back(s);
  return e;
}

Expression blockexpr(std::vector<Expression> exprs) {
  Expression e;
  e.type = Expression::Type::BLOCK;
  e.children.assign(exprs.begin(), exprs.end());
  return e;
}

Expression ifexpr(Expression cond, Expression a, Expression b) {
  Expression e;
  e.type = Expression::Type::IF;
  e.children.assign({cond, a, b});
  return e;
}

Expression printexpr(Expression v) {
  Expression e;
  e.type = Expression::Type::DEBUG_PRINT;
  e.children.push_back(v);
  return e;
}

class ProgramCounter final {
public:
  Blob *blob;
  long index;
  ProgramCounter(Blob *b, long i): blob(b), index(i) {}
  bool done() const { return static_cast<unsigned long>(index) >= blob->instructions.size(); }
  void incr() { index++; }
  void move(long i) { index = i; }
  Instruction &get() { return blob->instructions[index]; }
};

class VirtualMachine final {
public:
  std::vector<Value> evalstack;
  std::vector<ProgramCounter> retstack;
  std::vector<Table*> envstack;
  ProgramCounter pc;
  VirtualMachine(Table *t, const ProgramCounter &p): envstack({t}), pc(p) {}
  void run();
};

std::map<std::string, std::string*> internTable;

std::string *intern(const std::string &s) {
  if (internTable.find(s) == internTable.end()) {
    internTable[s] = new std::string(s);
  }
  return internTable[s];
}

std::string Instruction::str() {
  return ::str(type);
}

Value Table::get(std::string *key) const {
  auto pair = mapping.find(key);
  if (pair != mapping.end()) {
    return pair->second;
  } else {
    if (proto == nullptr) {
      throw error("No such name " + *key);
    } else {
      return proto->get(key);
    }
  }
}

void Table::declare(std::string *key, Value value) {
  if (mapping.find(key) == mapping.end()) {
    mapping[key] = value;
  } else {
    throw error("Already declared name " + *key);
  }
}

void Expression::compile(Blob &b) {
  switch (type) {
  case Type::NIL:
    b.instructions.push_back(Instruction(Instruction::Type::PUSH_NIL));
    break;
  case Type::INTEGER:
    b.instructions.push_back(Instruction(Instruction::Type::PUSH_INTEGER, integer));
    break;
  case Type::VARIABLE:
    b.instructions.push_back(Instruction(Instruction::Type::PUSH_VARIABLE, names[0]));
    break;
  case Type::LAMBDA: {
    Blob *blob = new Blob;
    blob->args = names;
    children[0].compile(*blob);
    b.instructions.push_back(Instruction(Instruction::Type::PUSH_FUNCTION, blob));
    break;
  }
  case Type::DECLARE:
    children[0].compile(b);
    b.instructions.push_back(Instruction(Instruction::Type::DECLARE_VARIABLE, names[0]));
    break;
  case Type::CALL:
    for (unsigned int i = 1; i < children.size(); i++) {
      children[i].compile(b);
    }
    children[0].compile(b);
    b.instructions.push_back(Instruction(Instruction::Type::CALL, children.size()-1));
    break;
  case Type::DEBUG_PRINT:
    children[0].compile(b);
    b.instructions.push_back(Instruction(Instruction::Type::DEBUG_PRINT));
    break;
  case Type::BLOCK:
    if (children.empty()) {
      b.instructions.push_back(Instruction(Instruction::Type::PUSH_NIL));
    } else {
      b.instructions.push_back(Instruction(Instruction::Type::BLOCK_START));
      for (unsigned long i = 0; i < children.size() - 1; i++) {
        std::cerr << "Compiling block i = " << i << " " << str(children[i].type) << std::endl;
        children[i].compile(b);
        b.instructions.push_back(Instruction(Instruction::Type::POP));
      }
      children.back().compile(b);
      b.instructions.push_back(Instruction(Instruction::Type::BLOCK_END));
    }
    break;
  case Type::IF:
    children[0].compile(b);
    long ifpos = b.instructions.size();
    b.instructions.push_back(Instruction(Instruction::Type::IF));
    children[1].compile(b);
    long elsepos = b.instructions.size();
    b.instructions.push_back(Instruction(Instruction::Type::ELSE));
    children[2].compile(b);
    b.instructions[ifpos].integer = elsepos + 1;
    b.instructions[elsepos].integer = b.instructions.size();
    break;
  }
}

void VirtualMachine::run() {
  std::cerr << "VirtualMachine::run" << std::endl;
  while (!(retstack.empty() && pc.done())) {
    if (pc.done()) {
      pc = retstack.back();
      retstack.pop_back();
      envstack.pop_back();
    } else {
      Instruction &i = pc.get();
      std::cerr << "-----" << std::endl;
      std::cerr << "i.type = " << str(i.type) << std::endl;
      std::cerr << "evalstack.size() = " << evalstack.size() << std::endl;
      std::cerr << "envstack.size() = " << envstack.size() << std::endl;
      switch (i.type) {
      case Instruction::Type::INVALID:
        error("Invalid instruction");
        break;
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
      case Instruction::Type::BLOCK_START:
        envstack.push_back(new Table(envstack.back()));
        pc.incr();
        break;
      case Instruction::Type::BLOCK_END:
        std::cerr << "envstack.size() = " << envstack.size() << std::endl;
        envstack.pop_back();
        pc.incr();
        break;
      case Instruction::Type::DECLARE_VARIABLE:
        envstack.back()->declare(i.name, evalstack.back());
        pc.incr();
        break;
      case Instruction::Type::PUSH_VARIABLE:
        evalstack.push_back(envstack.back()->get(i.name));
        pc.incr();
        break;
      case Instruction::Type::IF:
        if (evalstack.back().truthy()) {
          pc.incr();
        } else {
          pc.move(i.integer);
        }
        evalstack.pop_back();
        break;
      case Instruction::Type::ELSE:
        pc.move(i.integer);
        break;
      case Instruction::Type::PUSH_FUNCTION:
        evalstack.push_back(Value(Value::Type::FUNCTION, new Function(envstack.back(), i.blob)));
        pc.incr();
        break;
      case Instruction::Type::CALL:
        switch (evalstack.back().type) {
          case Value::Type::FUNCTION: {
            pc.incr();
            retstack.push_back(pc);
            auto f = evalstack.back().function();
            evalstack.pop_back();
            auto env = new Table(f->env);
            envstack.push_back(env);
            auto nargs = i.integer;
            if (static_cast<unsigned long>(nargs) != f->blob->args.size()) {
              error("Expected " + std::to_string(f->blob->args.size()) + " args but got " + std::to_string(nargs));
            }
            for (long j = 0; j < nargs; j++) {
              env->declare(f->blob->args[j], evalstack[evalstack.size() - nargs + j]);
            }
            evalstack.resize(evalstack.size() - nargs);
            pc = ProgramCounter(f->blob, 0);
            break;
          }
          default:
            error("Not calllable: " + str(evalstack.back().type));
            break;
        }
        break;
      // TODO: ---
      case Instruction::Type::TAILCALL:
        error("Not yet implemented");
      }
      // std::cerr << "2 i.type = " << str(i.type) << std::endl;
      std::cerr << "2 evalstack.size() = " << evalstack.size() << std::endl;
    }
  }
  std::cerr << "envstack.size() = " << envstack.size() << std::endl;
}

int main() {
  auto e = blockexpr({
    printexpr(intexpr(124124)),
    printexpr(intexpr(7)),
    printexpr(ifexpr(nilexpr(), intexpr(11111), intexpr(222222))),
    declexpr(intern("x"), intexpr(55371)),
    printexpr(varexpr(intern("x"))),
    declexpr(intern("f"), funcexpr({intern("a")}, blockexpr({
      printexpr(varexpr(intern("a"))),
    }))),
    callexpr(varexpr(intern("f")), {intexpr(777777)}),
    callexpr(varexpr(intern("f")), {intexpr(9999999999)}),
    printexpr(nilexpr())
  });
  Blob *blob = e.compile();
  std::cout << blob->str() << std::endl;
  VirtualMachine vm(new Table(), ProgramCounter(blob, 0));
  vm.run();
  std::cerr << "hi" << std::endl;
  return 0;
}
