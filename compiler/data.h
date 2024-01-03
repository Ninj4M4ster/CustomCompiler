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


typedef struct procedure_argument {
  std::string name;
  enum symbol_type type;
} ProcedureArgument;

typedef struct procedure_head {
  std::string name;
  std::vector<ProcedureArgument> arguments;
} ProcedureHead;

typedef struct command {

} Command;

typedef struct procedure {
  ProcedureHead head;
  std::shared_ptr<SymbolTable> symbol_table;
  std::vector<Command> commands;
} Procedure;

#endif //CUSTOMCOMPILER_COMPILER_DATA_H_
