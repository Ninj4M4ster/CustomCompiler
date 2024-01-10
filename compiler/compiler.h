#ifndef CUSTOMCOMPILER_COMPILER_COMPILER_H_
#define CUSTOMCOMPILER_COMPILER_COMPILER_H_

#include "code_generator.h"
#include "data.h"

class Compiler {
 public:
  Compiler();
  void setOutputFileName(std::string f_name);
  void compile();

  // procedures declarations
  void declareProcedure(std::vector<Command*> commands);
  void declareProcedureHead(std::string procedure_name, int line_number);

  // main declaration
  void declareMain(std::vector<Command*> commands);

  // variables declarations
  void declareVariable(std::string variable_name, int line_number);
  void declareVariable(std::string variable_name, long long int array_size, int line_number);

  // procedures arguments declarations
  void declareProcedureArgument(std::string variable_name, int line_number);
  void declareProcedureArrayArgument(std::string variable_name, int line_number);

  // procedures calls
  void addProcedureCallArgument(std::string variable_name, int line_number);
  void createProcedureCall(std::string procedure_name, int line_number);

  // commands creation
  Command* createAssignmentCommand(VariableContainer* left_var, DefaultExpression* expr, int line_number);
  Command* createIfThenElseBlock(Condition* cond,
                                 std::vector<Command*> then_commands,
                                 std::vector<Command*> else_commands,
                                 int line_number);
  Command* createIfThenElseBlock(Condition* cond, std::vector<Command*> then_commands, int line_number);
  Command* createWhileBlock(Condition* cond, std::vector<Command*> commands, int line_number);
  Command* createRepeatUntilBlock(Condition* cond, std::vector<Command*> commands, int line_number);
  Command* createProcedureCallCommand(int line_number);
  Command* createReadCommand(VariableContainer* var, int line_number);
  Command* createWriteCommand(VariableContainer* var, int line_number);

  // expressions creation
  // TODO(Jakub Drzewiecki): Line number might not be needed
  DefaultExpression* createDefaultExpression(VariableContainer* var, int line_number);
  DefaultExpression* createPlusExpression(VariableContainer* left_var, VariableContainer* right_var, int line_number);
  DefaultExpression* createMinusExpression(VariableContainer* left_var, VariableContainer* right_var, int line_number);
  DefaultExpression* createMultiplyExpression(VariableContainer* left_var, VariableContainer* right_var, int line_number);
  DefaultExpression* createDivideExpression(VariableContainer* left_var, VariableContainer* right_var, int line_number);
  DefaultExpression* createModuloExpression(VariableContainer* left_var, VariableContainer* right_var, int line_number);

  // conditions creation
  // TODO(Jakub Drzewiecki): Line number might not be needed
  Condition* createEqualCondition(VariableContainer* left_var, VariableContainer* right_var, int line_number);
  Condition* createNotEqualCondition(VariableContainer* left_var, VariableContainer* right_var, int line_number);
  Condition* createGreaterCondition(VariableContainer* left_var, VariableContainer* right_var, int line_number);
  Condition* createGreaterEqualCondition(VariableContainer* left_var, VariableContainer* right_var, int line_number);

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
  ProcedureHead curr_procedure_head_;

  // data and methods concerning procedure calls
  std::vector<ProcedureArgument> current_procedure_call_arguments_;
  ProcedureHead current_procedure_call_;
  void markProcedureArgumentNeedsInitialization(std::string arg_name);
  bool isProcedureArgumentMarked(std::string arg_name);

  std::shared_ptr<CodeGenerator> code_generator_;

  // containers for all procedures and main function
  std::vector<Procedure> procedures_;
  Procedure main_;

  std::string output_file_name_;
};

#endif  // CUSTOMCOMPILER_COMPILER_COMPILER_H_
