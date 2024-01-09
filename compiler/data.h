#ifndef CUSTOMCOMPILER_COMPILER_DATA_H_
#define CUSTOMCOMPILER_COMPILER_DATA_H_

#include <iostream>
#include <memory>
#include "symbol.h"
#include "symbol_table.h"

enum class variable_type {
  VAR,
  R_VAL,
  ARR,
  VARIABLE_INDEXED_ARR
};

typedef struct variable_container {
  variable_type type;
  virtual std::string getVariableName() {
    return "";
  };
  virtual std::string getIndexVariableName() {
    return "";
  };
  virtual long long int getValue() {
    return -1;
  }
} VariableContainer;

typedef struct variable : public VariableContainer {
  std::string var_name;
  std::string getVariableName() override {
    return var_name;
  };
} Variable;

typedef struct r_value : public VariableContainer {
  long long int value;
  long long getValue() override {
    return value;
  }
} RValue;

typedef struct array : public VariableContainer {
  std::string var_name;
  long long int index;
  std::string getVariableName() override {
    return var_name;
  };
  long long getValue() override {
    return index;
  }
} Array;

typedef struct variable_indexed_array : public VariableContainer {
  std::string var_name;
  std::string index_var_name;

  std::string getVariableName() override {
    return var_name;
  };
  std::string getIndexVariableName() override {
    return index_var_name;
  }
} VariableIndexedArray;

class Register {
 public:
  bool initialized_ = false;
  std::string register_name_;
  std::shared_ptr<VariableContainer> curr_variable = nullptr;
};

enum class expression_type {
  DEFAULT,
  PLUS,
  MINUS,
  MULTIPLY,
  DIVIDE,
  MODULO
};

// TODO(Jakub Drzewiecki): change expressions to two classes
/**
 * x := var_  (x - left side variable)
 *
 * get var_ into reg_?
 *
 * x is in reg_?
 */
class DefaultExpression {
 public:
  VariableContainer* var_;
  virtual std::vector<VariableContainer> neededVariablesInRegisters() {
    return {*var_};
  }
  virtual int needed_regs() {
    return 0;
  }
  // accumulator should be the last register
  virtual std::vector<std::string> calculateExpression(std::vector<std::shared_ptr<Register>> regs) {
    return {};
  }
 private:
  expression_type type = expression_type::DEFAULT;
};

/**
 * x := var_ + right_var_
 *
 * get var_ into reg_b
 * get right_var_ into reg_a
 * ADD b
 *
 * x is in reg a
 */
class PlusExpression : public DefaultExpression {
 public:
  VariableContainer* right_var_;
  std::vector<VariableContainer> neededVariablesInRegisters() override {
    return {*var_, *right_var_};
  }
  int needed_regs() override {
    return 1;
  }
  std::vector<std::string> calculateExpression(std::vector<std::shared_ptr<Register>> regs) override {
    std::shared_ptr<Register> var_register = regs.at(0);
    return {"ADD " + var_register->register_name_};
  }
 private:
  expression_type type = expression_type::PLUS;
};

/**
 * x := var_ + right_var_
 *
 * get right_var_ into reg_b
 * get var_ into reg_a
 * SUB b
 *
 * x is in reg a
 */
class MinusExpression : public DefaultExpression {
 public:
  VariableContainer* right_var_;
  std::vector<VariableContainer> neededVariablesInRegisters() override {
    return {*right_var_, *var_};
  }
  int needed_regs() override {
    return 1;
  }
  std::vector<std::string> calculateExpression(std::vector<std::shared_ptr<Register>> regs) override {
    std::shared_ptr<Register> right_var_register = regs.at(0);
    return {"SUB " + right_var_register->register_name_};
  }
 private:
  expression_type type = expression_type::MINUS;
};

/**
 * x := var_ * right_var_
 *
 * x is in reg a
 */
class MultiplyExpression : public DefaultExpression {
 public:
  VariableContainer* right_var_;
  std::vector<VariableContainer> neededVariablesInRegisters() override {
    return {*var_, *right_var_};
  }
 private:
  expression_type type = expression_type::MULTIPLY;
};

/**
 * x := var_ / right_var_
 *
 * x is in reg a
 */
class DivideExpression : public DefaultExpression {
 public:
  VariableContainer* right_var_;
  std::vector<VariableContainer> neededVariablesInRegisters() override {
    return {*var_, *right_var_};
  }
 private:
  expression_type type = expression_type::DIVIDE;
};

/**
 * x := var_ % right_var_
 *
 * x is in reg a
 */
class ModuloExpression : public DefaultExpression {
 public:
  VariableContainer* right_var_;
  std::vector<VariableContainer> neededVariablesInRegisters() override {
    return {*var_, *right_var_};
  }
 private:
  expression_type type = expression_type::MODULO;
};

/**
 * a ? b
 * ---------
 * EQ: a - b == 0 && b - a == 0
 *
 * LOAD b_add
 * PUT b
 * LOAD a_add
 * PUT c
 * SUB b
 * JPOS next_block  (a - b > 0 -> a != b; a - b == 0 -> b - a ? 0)
 * GET b
 * SUB c
 * JPOS next_block (b - a > 0 -> a != b; b - a == 0 -> a == b)
 * equal block
 * next_block
 * -----------------
 * NEQ: a - b != 0 || b - a != 0
 *
 * LOAD b_add
 * PUT b
 * LOAD a_add
 * PUT c
 * SUB b
 * JPOS nequal_block  (a - b > 0 -> a != b; a - b == 0 -> b - a ? 0)
 * GET b
 * SUB c
 * JZERO next_block  (b - a == 0 -> a == b ; b - a > 0 -> a != b)
 * nequal_block
 * next_block
 * ------------------
 * GT:  a > b -> !(a <= b) -> !(a < b + 1) -> !(b + 1 - a > 0)
 *
 * LOAD a_add
 * PUT b
 * LOAD b_add
 * INC a
 * SUB b  (b + 1 - a)
 * JPOS next_block
 * gt_block
 * next_block
 * -------------------------
 * GE: a >= b -> a + 1 > b -> a + 1 - b > 0
 *
 * LOAD b_add
 * PUT b
 * LOAD a_add
 * INC a
 * SUB b  (a + 1 - b)
 * JZERO next_block  (a + 1 - b == 0 -> a + 1 <= b)
 * ge_block
 * next_block
 */
enum class condition_type {
  EQ,
  NEQ,
  GT,
  GE
};

// TODO(Jakub Drzewiecki): Optimize conditions if they are always false/true
class Condition {
 public:
  condition_type type_;
  VariableContainer* left_var_;
  VariableContainer* right_var_;
};

// TODO(Jakub Drzewiecki): Add system for checking if argument is not used uninitialized illegally in procedure
// TODO(Jakub Drzewiecki): To pass args by reference, it is needed to pass memory addresses of the arguments
typedef struct procedure_argument {
  std::string name;
  enum symbol_type type;
  bool needs_initialization_before_call;
} ProcedureArgument;

typedef struct procedure_head {
  std::string name;
  std::vector<ProcedureArgument> arguments;
} ProcedureHead;

enum class command_type {
  ASSIGNMENT,
  IF_ELSE,
  WHILE,
  REPEAT,
  PROC_CALL,
  READ,
  WRITE
};

class Command {
 public:
  enum command_type type;
};

/**
 * x := a ? b
 * ? - expression
 * y_add - address of y variable
 *
 * LOAD a_add
 * ? b (store expression result in R_a)
 * STORE x_add
 */
class AssignmentCommand : public Command {
 public:
  VariableContainer left_var_;
  DefaultExpression expression_;
};

/**
 * if else then:
 * condition target block - then_commands
 * condition next block - else_commands
 *
 * cond-block
 * then_commands
 * JUMP next_block
 * else_commands
 * next_block
 *
 * ----------------
 * if then:
 * condition target block - then commands
 * condition next block - next block
 *
 * cond-block
 * then_commands
 * next_block
 */
class IfElseCommand : public Command {
 public:
  Condition cond_;
  std::vector<Command*> then_commands_;
  std::vector<Command*> else_commands_;
};

/**
 * condition target block - commands
 * condition next block - next_block
 *
 * cond-block
 * commands
 * JUMP cond-block
 * next_block
 */
class WhileCommand : public Command {
 public:
  Condition cond_;
  std::vector<Command*> commands_;
};

/**
 * condition target block - loop_end
 * condition next block - commands
 *
 * cond-block
 * loop_end
 * JUMP next_block
 * commands
 * JUMP cond_block
 * next_block
 */
class RepeatUntilCommand : public Command {
 public:
  Condition cond_;
  std::vector<Command*> commands_;
};

/**
 * firstly store all current registers that hold values in memory
 *
 * STRK h  (store current line number)
 * JUMP proc_start
 * procedure
 * INC h
 * INC h
 * GET h
 * JUMPR h
 *
 */
class ProcedureCallCommand : public Command {
 public:
  ProcedureHead proc_call_;
};

/**
 * READ a:
 *
 * READ
 * STORE a_add
 */
class ReadCommand : public Command {
 public:
  VariableContainer var_;
};

/**
 * WRITE a:
 *
 * LOAD a_add
 * WRITE
 */
class WriteCommand : public Command {
 public:
  VariableContainer written_value_;
};

typedef struct procedure {
  ProcedureHead head;
  std::shared_ptr<SymbolTable> symbol_table;
  std::vector<Command*> commands;
} Procedure;

#endif  // CUSTOMCOMPILER_COMPILER_DATA_H_
