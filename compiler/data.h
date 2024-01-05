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
} VariableContainer;

typedef struct variable : public VariableContainer {
  std::string var_name;
  std::string getVariableName() override {
    return var_name;
  };
} Variable;

typedef struct r_value : public VariableContainer {
  long long int value;
} RValue;

typedef struct array : public VariableContainer {
  std::string var_name;
  long long int index;
  std::string getVariableName() override {
    return var_name;
  };
} Array;

typedef struct variable_indexed_array : public VariableContainer {
  std::string var_name;
  std::string index_var_name;

  std::string getVariableName() override {
    return var_name;
  };
} VariableIndexedArray;


enum class expression_type {
  DEFAULT,
  PLUS,
  MINUS,
  MULTIPLY,
  DIVIDE,
  MODULO
};

// TODO(Jakub Drzewiecki): change expressions to two classes
class DefaultExpression {
 public:
  VariableContainer* var_;
 private:
  expression_type type = expression_type::DEFAULT;
};

class PlusExpression : public DefaultExpression {
 public:
  VariableContainer* right_var_;
 private:
  expression_type type = expression_type::PLUS;
};

class MinusExpression : public DefaultExpression {
 public:
  VariableContainer* right_var_;
 private:
  expression_type type = expression_type::MINUS;
};

class MultiplyExpression : public DefaultExpression {
 public:
  VariableContainer* right_var_;
 private:
  expression_type type = expression_type::MULTIPLY;
};

class DivideExpression : public DefaultExpression {
 public:
  VariableContainer* right_var_;
 private:
  expression_type type = expression_type::DIVIDE;
};

class ModuloExpression : public DefaultExpression {
 public:
  VariableContainer* right_var_;
 private:
  expression_type type = expression_type::MODULO;
};

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

class AssignmentCommand : public Command {
 public:
  VariableContainer left_var_;
  DefaultExpression expression_;
  enum command_type type = command_type::ASSIGNMENT;
};

class IfElseCommand : public Command {
 public:
  Condition cond_;
  std::vector<Command*> then_commands_;
  std::vector<Command*> else_commands_;
  enum command_type type = command_type::IF_ELSE;
};

class WhileCommand : public Command {
 public:
  Condition cond_;
  std::vector<Command*> commands_;
  enum command_type type = command_type::WHILE;
};

class RepeatUntilCommand : public Command {
 public:
  Condition cond_;
  std::vector<Command*> commands_;
  enum command_type type = command_type::REPEAT;
};

class ProcedureCallCommand : public Command {
 public:
  ProcedureHead proc_call_;
  enum command_type type = command_type::PROC_CALL;
};

class ReadCommand : public Command {
 public:
  VariableContainer var_;
  enum command_type type = command_type::READ;
};

class WriteCommand : public Command {
 public:
  VariableContainer written_value_;
  enum command_type type = command_type::WRITE;
};

typedef struct procedure {
  ProcedureHead head;
  std::shared_ptr<SymbolTable> symbol_table;
  std::vector<Command*> commands;
} Procedure;

#endif  // CUSTOMCOMPILER_COMPILER_DATA_H_
