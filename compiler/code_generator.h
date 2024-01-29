#ifndef CUSTOMCOMPILER_COMPILER_CODE_GENERATOR_H_
#define CUSTOMCOMPILER_COMPILER_CODE_GENERATOR_H_

#include "data.h"

class GraphNode {
 public:
  std::vector<Command*> commands;
  std::shared_ptr<Condition> cond = nullptr;
  std::shared_ptr<GraphNode> left_node = nullptr;
  std::shared_ptr<GraphNode> right_node = nullptr;

  std::vector<std::string> code_list_;
  std::vector<std::string> registers_saving_code_list_;

  // start line of current node
  long long int start_line_ = -1;
  long long int condition_start_line_ = -1;
  long long int node_end_line_ = -1;
  // target of jump command, that should be added after all commands from this, left and right node; it should target start of the code block
  std::shared_ptr<GraphNode> jump_line_target = nullptr;
  // target of jump command, that should be added after all commands from this, left and right node; it should target start of the condition block
  std::shared_ptr<GraphNode> jump_condition_target = nullptr;
  // length of code in this node
  long long int node_length_ = 0;

  std::string proc_name;
  bool should_save_registers_after_code = false;

  std::vector<std::shared_ptr<Register>> regs_prepared_for_condition;
};

class CodeGenerator {
 public:
  CodeGenerator();
  void generateFlowGraph(Procedure main, std::vector<Procedure> procedures);
  void generateCode();
  std::vector<std::shared_ptr<GraphNode>> getGraphs();

 private:
  std::shared_ptr<GraphNode> generateSingleFlowGraph(Procedure proc);
  void process_commands(std::vector<Command*> comms, std::shared_ptr<GraphNode> curr_node);
  std::shared_ptr<GraphNode> start_node;
  std::shared_ptr<SymbolTable> current_symbol_table_;
  std::vector<std::shared_ptr<SymbolTable>> symbol_tables_;
  std::vector<std::shared_ptr<GraphNode>> procedures_start_nodes_;
  std::vector<std::string> procedures_names_;

  void generateCodePreorder(std::shared_ptr<GraphNode> node);
  std::shared_ptr<Register> accumulator_;
  std::vector<std::shared_ptr<Register>> registers_;

  long long int current_start_line_ = 0;
  bool generate_jump_to_main_ = false;
  void generateProcedureStart(std::shared_ptr<GraphNode> node);
  void generateProcedureEnd(std::shared_ptr<GraphNode> node);

  // registers management
  void saveVariableFromRegister(std::shared_ptr<Register> reg,
                                std::shared_ptr<Register> other_free_reg,
                                std::shared_ptr<GraphNode> node,
                                bool keep_variable);
  void moveAccumulatorToFreeRegister(std::shared_ptr<GraphNode> node);
  std::shared_ptr<Register> findFreeRegister(std::shared_ptr<GraphNode> node);
  void getValueIntoRegister(size_t value, std::shared_ptr<Register> reg, std::shared_ptr<GraphNode> node);
  std::shared_ptr<Register> checkVariableAlreadyLoaded(VariableContainer* var);
  std::shared_ptr<Register> loadVariable(VariableContainer* var,
                                         std::shared_ptr<Register> target_reg,
                                         std::shared_ptr<GraphNode> node,
                                         bool use_saved_variables);

  std::vector<std::shared_ptr<Register>> saveRegistersState();
  void loadRegistersState(std::vector<std::shared_ptr<Register>> saved_regs);

  void saveRegistersValues(std::shared_ptr<GraphNode> node);

  // commands handling
  void handleAssignmentCommand(AssignmentCommand* command, std::shared_ptr<GraphNode> node);
  void handleReadCommand(ReadCommand* command, std::shared_ptr<GraphNode> node);
  void handleWriteCommand(WriteCommand* command, std::shared_ptr<GraphNode> node);
  void handleProcedureCallCommand(ProcedureCallCommand* command, std::shared_ptr<GraphNode> node);

  // conditions handling
  void prepareCondition(std::shared_ptr<GraphNode> node);
  void generateCondition(std::shared_ptr<GraphNode> node);

  void saveRegisterAfterAssignmentIfNeeded(AssignmentCommand *command,
                                           std::shared_ptr<Register> reg_with_result,
                                           std::shared_ptr<GraphNode> node);
};

#endif //CUSTOMCOMPILER_COMPILER_CODE_GENERATOR_H_
