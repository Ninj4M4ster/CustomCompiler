#include "flow_graph.h"

FlowGraph::FlowGraph() {
  std::vector<std::string> register_names{"b", "c", "d", "e", "f", "g", "h"};
  accumulator_ = std::make_shared<Register>();
  accumulator_->register_name_ = "a";
  for(auto name : register_names) {
    std::shared_ptr<Register> new_reg;
    new_reg->register_name_ = name;
    registers_.push_back(new_reg);
  }
}

void FlowGraph::generateFlowGraph(Procedure main, std::vector<Procedure> procedures) {
  for(auto proc : procedures) {
    procedures_start_nodes_.push_back(generateSingleFlowGraph(proc));
  }
  start_node = generateSingleFlowGraph(main);

}

void FlowGraph::generateCode() {
  for(auto proc_start : procedures_start_nodes_) {
    generateCodePreorder(proc_start);
  }
  generateCodePreorder(start_node);
}

std::shared_ptr<GraphNode> FlowGraph::generateSingleFlowGraph(Procedure proc) {
  std::shared_ptr<GraphNode> curr_node = std::make_shared<GraphNode>();
  process_commands(proc.commands, curr_node);
  return curr_node;
}

void FlowGraph::process_commands(std::vector<Command *> comms,
                                 std::shared_ptr<GraphNode> curr_node) {
  for(auto comm : comms) {
    if(comm->type == command_type::REPEAT) {
      // TODO(Jakub Drzewiecki): Consider negating condition and simplifying nodes creation
      RepeatUntilCommand * new_comm = static_cast<RepeatUntilCommand*>(comm);
      curr_node->cond = std::make_shared<Condition>(new_comm->cond_);
      // create left branch - empty branch with jump command
      std::shared_ptr<GraphNode> left_node = std::make_shared<GraphNode>();
      curr_node->left_node = left_node;
      // create right branch
      std::shared_ptr<GraphNode> right_node = std::make_shared<GraphNode>();
      curr_node->right_node = right_node;
      process_commands(new_comm->commands_, right_node);
      right_node->jump_condition_target = curr_node;
      // add next block and update left branch target
      std::shared_ptr<GraphNode> next_code = std::make_shared<GraphNode>();
      curr_node->left_node->jump_line_target = next_code;
      // pass next code branch to furthermost right branch
      while(right_node->right_node != nullptr) {
        right_node = right_node->right_node;
      }
      right_node->right_node = next_code;
      curr_node = next_code;
    } else if(comm->type == command_type::WHILE) {
      WhileCommand * new_comm = static_cast<WhileCommand*>(comm);
      curr_node->cond = std::make_shared<Condition>(new_comm->cond_);
      // create left branch
      std::shared_ptr<GraphNode> left_node = std::make_shared<GraphNode>();
      curr_node->left_node = left_node;
      process_commands(new_comm->commands_, left_node);
      left_node->jump_condition_target = curr_node;
      // create next code branch
      std::shared_ptr<GraphNode> right_node = std::make_shared<GraphNode>();
      curr_node->right_node = right_node;
      curr_node = right_node;
    } else if(comm->type == command_type::IF_ELSE) {
      IfElseCommand * new_comm = static_cast<IfElseCommand*>(comm);
      curr_node->cond = std::make_shared<Condition>(new_comm->cond_);
      // create left branch
      std::shared_ptr<GraphNode> left_node = std::make_shared<GraphNode>();
      curr_node->left_node = left_node;
      process_commands(new_comm->then_commands_, left_node);
      if(!new_comm->else_commands_.empty()) {
        // create right branch
        std::shared_ptr<GraphNode> right_node = std::make_shared<GraphNode>();
        curr_node->right_node = right_node;
        process_commands(new_comm->else_commands_, right_node);
        // create next code branch that will run after both branches
        std::shared_ptr<GraphNode> next_code = std::make_shared<GraphNode>();
        left_node->jump_line_target = next_code;
        // pass next code branch to furthermost right branch
        while(right_node->right_node != nullptr) {
          right_node = right_node->right_node;
        }
        right_node->right_node = next_code;
        curr_node = next_code;
      } else {
        // right branch is following code from currently processed code
        std::shared_ptr<GraphNode> next_code = std::make_shared<GraphNode>();
        curr_node = next_code;
      }
    } else {
      curr_node->commands.push_back(comm);
    }
  }
}

// TODO(Jakub Drzewiecki): Pass current symbol table to this function
// TODO(Jakub Drzewiecki): Missing handling one free register always
void FlowGraph::generateCodePreorder(std::shared_ptr<GraphNode> node) {
  // generate code for current node
  for(auto comm : node->commands) {
    if(comm->type == command_type::ASSIGNMENT) {
      AssignmentCommand* assignment_command = static_cast<AssignmentCommand*>(comm);
      handleAssignmentCommand(assignment_command, node);
    }  // assignment command
    else if(comm->type == command_type::READ) {

    } else if(comm->type == command_type::WRITE) {

    } else if(comm->type == command_type::PROC_CALL) {  // pass variable arguments memory start to procedure memory for given arguments

    }

  }

  // put condition to code
  // generate left node code
  // generate right node code
  // TODO(Jakub Drzewiecki): Handle registers states after left/right branch if there is if/else block following
  if(node->left_node != nullptr)
    generateCodePreorder(node->left_node);
  if(node->right_node != nullptr)
    generateCodePreorder(node->right_node);
}

void FlowGraph::saveVariableFromRegister(std::shared_ptr<Register> reg, std::shared_ptr<GraphNode> node) {

}

void FlowGraph::moveAccumulatorToFreeRegister(std::shared_ptr<GraphNode> node) {

}

// TODO(Jakub Drzewiecki): Important rule: there should be always one register free - either accumulator or in rest of the pool
std::shared_ptr<Register> FlowGraph::findFreeRegister() {

}

void FlowGraph::getValueIntoRegister(long long value, std::shared_ptr<Register> reg, std::shared_ptr<GraphNode> node) {
  node->code_list_.push_back("RST " + reg->register_name_);
  unsigned long long int bit = (1LU << 63);
  while(!(value & bit)) {
    bit >>= 1;
  }
  bool first = true;
  while(bit != 0) {
    if(!first) {
      node->code_list_.push_back("SHL " + reg->register_name_);
    }
    if(bit & value) {
      node->code_list_.push_back("INC " + reg->register_name_);
    }
    first = false;
    bit >>= 1;
  }
}

std::shared_ptr<Register> FlowGraph::loadVariable(VariableContainer var, std::shared_ptr<GraphNode> node) {
  std::shared_ptr<Register> chosen_reg = nullptr;
  // check if variable is not already stored in one of registers
  for(auto reg : registers_) {
    if(reg->curr_variable) {
      if(reg->curr_variable->getVariableName() == var.getVariableName()) { // check r values
        chosen_reg = reg;
        break;
      }
    }
  }
  // TODO(Jakub Drzewiecki): Add handling process arguments variables
  if(!chosen_reg) {
    // TODO(Jakub Drzewiecki): Check if A register is free and can be used to load values
    moveAccumulatorToFreeRegister(node);
    chosen_reg = findFreeRegister();
    std::shared_ptr<Register> empty_reg_for_index_var;
    chosen_reg->curr_variable = std::make_shared<VariableContainer>(var);
    unsigned long long int target_val = 0;
    bool is_procedure_argument = false;
    // algorithm for creating binary representation of mem address
    if(var.type == variable_type::R_VAL) {
      target_val = var.getValue();
    } else if(var.type == variable_type::VAR) {
      auto sym = symbol_table_->findSymbol(var.getVariableName());
      target_val = sym->mem_start;
    } else if(var.type == variable_type::ARR) {
      auto sym = symbol_table_->findSymbol(var.getVariableName());
      target_val = sym->mem_start + var.getValue();
    } else {
      auto sym = symbol_table_->findSymbol(var.getVariableName());
      target_val = sym->mem_start;
      auto index_sym = symbol_table_->findSymbol(var.getIndexVariableName());
      unsigned long long int index_var_target_val = index_sym->mem_start;
      empty_reg_for_index_var = findFreeRegister();
      Variable index_var;
      index_var.var_name = var.getIndexVariableName();
      index_var.type = variable_type::VAR;
      empty_reg_for_index_var->curr_variable = std::make_shared<VariableContainer>(index_var);
      getValueIntoRegister(index_var_target_val, empty_reg_for_index_var, node);
      node->code_list_.push_back("LOAD " + empty_reg_for_index_var->register_name_);
      node->code_list_.push_back("PUT " + empty_reg_for_index_var->register_name_);
    }
    getValueIntoRegister(target_val, chosen_reg, node);
    if(var.type != variable_type::R_VAL) {
      if (var.type == variable_type::VARIABLE_INDEXED_ARR) {  // add index offset
        node->code_list_.push_back("GET " + chosen_reg->register_name_);
        node->code_list_.push_back("ADD " + empty_reg_for_index_var->register_name_);
        node->code_list_.push_back("PUT " + chosen_reg->register_name_);
      }
      node->code_list_.push_back("LOAD " + chosen_reg->register_name_);
      node->code_list_.push_back("PUT " + chosen_reg->register_name_);
    }
  }
  return chosen_reg;
}

void FlowGraph::loadVariableToAccumulator(VariableContainer var, std::shared_ptr<GraphNode> node) {
  std::shared_ptr<Register> chosen_reg = nullptr;\
  // check if variable is not already stored in one of registers
  for(auto reg : registers_) {
    if(reg->curr_variable) {
      if(reg->curr_variable->getVariableName() == var.getVariableName()) { // check r values
        chosen_reg = reg;
        break;
      }
    }
  }
  // TODO(Jakub Drzewiecki): Add handling process arguments variables

  moveAccumulatorToFreeRegister(node);
  if(chosen_reg) {
    node->code_list_.push_back("GET " + chosen_reg->register_name_);
  } else {
    std::shared_ptr<Register> empty_reg_for_index_var;
    accumulator_->curr_variable = std::make_shared<VariableContainer>(var);
    unsigned long long int target_val = 0;
    bool is_procedure_argument = false;
    // algorithm for creating binary representation of mem address
    if(var.type == variable_type::R_VAL) {
      target_val = var.getValue();
    } else if(var.type == variable_type::VAR) {
      auto sym = symbol_table_->findSymbol(var.getVariableName());
      target_val = sym->mem_start;
    } else if(var.type == variable_type::ARR) {
      auto sym = symbol_table_->findSymbol(var.getVariableName());
      target_val = sym->mem_start + var.getValue();
    } else {
      auto sym = symbol_table_->findSymbol(var.getVariableName());
      target_val = sym->mem_start;
      auto index_sym = symbol_table_->findSymbol(var.getIndexVariableName());
      unsigned long long int index_var_target_val = index_sym->mem_start;
      empty_reg_for_index_var = findFreeRegister();
      Variable index_var;
      index_var.var_name = var.getIndexVariableName();
      index_var.type = variable_type::VAR;
      empty_reg_for_index_var->curr_variable = std::make_shared<VariableContainer>(index_var);
      getValueIntoRegister(index_var_target_val, empty_reg_for_index_var, node);
      node->code_list_.push_back("LOAD " + empty_reg_for_index_var->register_name_);
      node->code_list_.push_back("PUT " + empty_reg_for_index_var->register_name_);
    }
    getValueIntoRegister(target_val, accumulator_, node);
    if(var.type != variable_type::R_VAL) {
      if (var.type == variable_type::VARIABLE_INDEXED_ARR) {  // add index offset
        node->code_list_.push_back("ADD " + empty_reg_for_index_var->register_name_);
      }
      node->code_list_.push_back("LOAD " + accumulator_->register_name_);
    }
  }
}

void FlowGraph::handleAssignmentCommand(AssignmentCommand *command, std::shared_ptr<GraphNode> node) {
  std::vector<VariableContainer> needed_variables = command->expression_.neededVariablesInRegisters();
  // load given variables into registers
  std::vector<std::shared_ptr<Register>> prepared_registers;
  for(int i = 0; i < needed_variables.size() - 1; i++) {
    prepared_registers.push_back(loadVariable(needed_variables.at(i), node));
  }
  // load last variable to accumulator
  loadVariableToAccumulator(needed_variables.at(needed_variables.size() - 1), node);
  prepared_registers.push_back(accumulator_);  // accumulator is last register in vector
  // expression should handle creating rest of code using prepared registers
  // TODO(Jakub Drzewiecki): Might need a way to handle adding jump commands
  std::vector<std::string> generated_commands =
      command->expression_.calculateExpression(prepared_registers);
  accumulator_->curr_variable = std::make_shared<VariableContainer>(command->left_var_);
  for(auto generated_code : generated_commands) {
    node->code_list_.push_back(generated_code);
  }
}
