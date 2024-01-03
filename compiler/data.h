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
  virtual std::string getVariableName();
} VariableContainer;

typedef struct variable : public VariableContainer {
  std::string var_name;
} Variable;

typedef struct r_value : public VariableContainer {
  long long int value;
} RValue;

typedef struct array : public VariableContainer {
  std::string var_name;
  long long int index;
} Array;

typedef struct variable_indexed_array : public VariableContainer {
  std::string var_name;
  std::string index_var_name;
} VariableIndexedArray;

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
