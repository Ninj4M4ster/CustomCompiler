#ifndef CUSTOMCOMPILER_COMPILER_COMPILER_H_
#define CUSTOMCOMPILER_COMPILER_COMPILER_H_

#include "flow_graph.h"
#include "data.h"

class Compiler {
 public:
  Compiler();
  void setOutputFileName(std::string f_name);

  // procedures declarations
  void declareProcedure();
  void declareProcedureHead(std::string procedure_name, int line_number);

  // variables declarations
  void declareVariable(std::string variable_name, int line_number);
  void declareVariable(std::string variable_name, long long int array_size, int line_number);

  // procedures arguments declarations
  void declareProcedureArgument(std::string variable_name, int line_number);
  void declareProcedureArrayArgument(std::string variable_name, int line_number);

  // variables obtaining
  VariableContainer* getVariable(long long int value, int line_number);
  VariableContainer* getVariable(std::string variable_name, int line_number);
  VariableContainer* getVariable(std::string variable_name, long long int index, int line_number);
  VariableContainer* getVariable(std::string variable_name, std::string index_variable_name, int line_number);
  VariableContainer* checkVariableInitialization(VariableContainer* var, int line_number);

 private:
  // current symbol table used for local declarations, passed to functions objects
  std::shared_ptr<SymbolTable> current_symbol_table_;

  // variables declaration handling
  size_t curr_memory_offset_ = 0;
  static constexpr size_t k_max_mem_offset = 0x4000000000000000U;
  Symbol createSymbol(std::string symbol_name, enum symbol_type type);
  void setSymbolBounds(Symbol & symbol, long long int mem_len, int line_number);

  // temporary data used in procedures/main function declaration
  std::vector<ProcedureArgument> current_procedure_arguments_;
  std::vector<Command> current_commands_;
  ProcedureHead curr_procedure_head_;

  std::shared_ptr<FlowGraph> flow_graph_;

  // containers for all procedures and main function
  std::vector<Procedure> procedures_;
  Procedure main_;

  std::string output_file_name_;
};

#endif  // CUSTOMCOMPILER_COMPILER_COMPILER_H_
