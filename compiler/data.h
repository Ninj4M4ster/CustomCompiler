
#ifndef CUSTOMCOMPILER_COMPILER_DATA_H_
#define CUSTOMCOMPILER_COMPILER_DATA_H_

#include <iostream>
#include <memory>
#include "symbol.h"
#include "symbol_table.h"

typedef struct variable {
  Symbol symbol;
} Variable;

typedef struct procedure_argument {
  std::string name;
  enum symbol_type type;
} ProcedureArgument;

typedef struct procedure_head {
  std::string name;
  std::vector<ProcedureArgument> arguments;
} ProcedureHead;

enum procedure_type {
  MAIN,
  PROC
};

typedef struct command {

} Command;

typedef struct procedure {
  ProcedureHead head;
  enum procedure_type proc_type;
  std::shared_ptr<SymbolTable> symbol_table;
  std::vector<Command> commands;
} Procedure;

#endif //CUSTOMCOMPILER_COMPILER_DATA_H_
