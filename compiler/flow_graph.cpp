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
    current_symbol_table_ = proc.symbol_table;
    procedures_start_nodes_.push_back(generateSingleFlowGraph(proc));
  }
  current_symbol_table_ = main.symbol_table;
  start_node = generateSingleFlowGraph(main);

}

// add
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

// TODO(Jakub Drzewiecki): Missing handling one free register always
void FlowGraph::generateCodePreorder(std::shared_ptr<GraphNode> node) {
  // generate code for current node
  for(auto comm : node->commands) {
    if(comm->type == command_type::ASSIGNMENT) {
      AssignmentCommand* assignment_command = static_cast<AssignmentCommand*>(comm);
      handleAssignmentCommand(assignment_command, node);
    }  // assignment command
    else if(comm->type == command_type::READ) {
      ReadCommand* read_command = static_cast<ReadCommand*>(comm);
      handleReadCommand(read_command, node);
    } else if(comm->type == command_type::WRITE) {
      WriteCommand* write_command = static_cast<WriteCommand*>(comm);
      handleWriteCommand(write_command, node);
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

// TODO(Jakub Drzewiecki): Possible missing handling which variable is in accumulator (could be other registers too)
void FlowGraph::saveVariableFromRegister(std::shared_ptr<Register> reg,
                                         std::shared_ptr<Register> other_free_reg,
                                         std::shared_ptr<GraphNode> node) {
  if(reg->variable_saved_) {
    reg->curr_variable = nullptr;
    reg->currently_used_ = false;
    return;
  }
  auto var = reg->curr_variable;
  if(var->type == variable_type::R_VAL || var->type == variable_type::VARIABLE_INDEXED_ARR) {
    reg->curr_variable = nullptr;
    reg->currently_used_ = false;
    reg->variable_saved_ = true;
    return;
  }
  auto sym =
      current_symbol_table_->findSymbol(reg->curr_variable->getVariableName());
  if(var->type == variable_type::ARR && sym->type == symbol_type::PROC_ARRAY_ARGUMENT) {
    getValueIntoRegister(sym->mem_start, other_free_reg, node);
    node->code_list_.push_back("LOAD " + other_free_reg->register_name_);
    node->code_list_.push_back("PUT " + other_free_reg->register_name_);
    getValueIntoRegister(var->getValue(), accumulator_, node);
    node->code_list_.push_back("ADD " + other_free_reg->register_name_);
    node->code_list_.push_back("PUT " + other_free_reg->register_name_);
    node->code_list_.push_back("GET " + reg->register_name_);
    node->code_list_.push_back("STORE " + other_free_reg->register_name_);
    other_free_reg->curr_variable = nullptr;
    other_free_reg->currently_used_ = false;
    other_free_reg->variable_saved_ = true;
  } else if(sym->type == symbol_type::PROC_ARGUMENT) {
    getValueIntoRegister(sym->mem_start, other_free_reg, node);
    node->code_list_.push_back("LOAD " + other_free_reg->register_name_);
    node->code_list_.push_back("PUT " + other_free_reg->register_name_);
    node->code_list_.push_back("GET " + reg->register_name_);
    node->code_list_.push_back("STORE " + other_free_reg->register_name_);
    other_free_reg->curr_variable = nullptr;
    other_free_reg->currently_used_ = false;
    other_free_reg->variable_saved_ = true;
  } else {
    node->code_list_.push_back("GET " + reg->register_name_);
    getValueIntoRegister(sym->mem_start, reg, node);
    node->code_list_.push_back("STORE " + reg->register_name_);
  }
  accumulator_->curr_variable = reg->curr_variable;
  accumulator_->currently_used_ = false;
  accumulator_->variable_saved_ = true;
  reg->curr_variable = nullptr;
  reg->currently_used_ = false;
  reg->variable_saved_ = true;
}

// This method ignores registers that are currently used
void FlowGraph::moveAccumulatorToFreeRegister(std::shared_ptr<GraphNode> node) {
  if(accumulator_->variable_saved_) {
    accumulator_->currently_used_ = false;
    accumulator_->curr_variable = nullptr;
    return;
  }
  int regs_with_unsaved_vals = 0;
  std::shared_ptr<Register> chosen_reg = nullptr;
  std::shared_ptr<Register> second_chosen_reg = nullptr;
  for(auto reg : registers_) {
    if(!reg->variable_saved_) {
      regs_with_unsaved_vals++;
    }
    if(reg->variable_saved_ && !chosen_reg) {
      chosen_reg = reg;
    }
    if(reg->variable_saved_ && chosen_reg && !second_chosen_reg) {
      second_chosen_reg = reg;
    }
  }
  chosen_reg->curr_variable = accumulator_->curr_variable;
  chosen_reg->variable_saved_ = accumulator_->variable_saved_;
  chosen_reg->currently_used_ = false;
  accumulator_->curr_variable = nullptr;
  accumulator_->variable_saved_ = true;
  node->code_list_.push_back("PUT " + chosen_reg->register_name_);
  if(7 - regs_with_unsaved_vals == 2 && !chosen_reg->variable_saved_) {  // leaving one reg for use
    // choose reg with unsaved value that is not currently used
    std::shared_ptr<Register> reg_for_save = nullptr;
    for(auto reg : registers_) {
      if(!reg->variable_saved_ && !reg->currently_used_) {
        reg_for_save = reg;
        break;
      }
    }
    saveVariableFromRegister(reg_for_save, second_chosen_reg, node);
  }
}

/**
 * This method is meant to find a register that can be used later
 * in any computations or to read/write variable.
 * It cannot be used if any of the registers is currently used in computations -
 * all computations have to be finished before using this method.
 *
 * @param node Current flow graph node in which code might be generated.
 * @return Register that can be used later in computations and is not already chosen for them.
 */
std::shared_ptr<Register> FlowGraph::findFreeRegister(std::shared_ptr<GraphNode> node) {
  std::shared_ptr<Register> chosen_reg = nullptr;
  for(auto reg : registers_) {
    if(!reg->curr_variable && !reg->currently_used_) {
      chosen_reg = reg;
      break;
    }
  }
  if(!chosen_reg) {  // no free registers with no values
    for(auto reg : registers_) {
      if(reg->variable_saved_ && !reg->currently_used_ && !chosen_reg) {
        chosen_reg = reg;
        break;
      }
    }
    // no free registers with saved values - save one of registers and choose it
    if(!chosen_reg) {
      for(auto reg : registers_) {
        if(!reg->variable_saved_ && !reg->currently_used_) {
          chosen_reg = reg;
          break;
        }
      }
      // chosen_reg will be saved, reg_for_help is going to be used to help saving this register
      // currently_used flag can be omitted, because registers cannot have any computations ongoing,
      // and it will not be returned
      std::shared_ptr<Register> reg_for_help = nullptr;
      for(auto reg : registers_) {
        if(reg->variable_saved_) {
          reg_for_help = reg;
          break;
        }
      }
      saveVariableFromRegister(chosen_reg, reg_for_help, node);
    }
  }
  return chosen_reg;
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

std::shared_ptr<Register> FlowGraph::checkVariableAlreadyLoaded(VariableContainer var) {
  std::shared_ptr<Register> chosen_reg = nullptr;
  if(var.type == variable_type::VARIABLE_INDEXED_ARR)
    return chosen_reg;
  for(auto reg : registers_) {
    if(reg->curr_variable) {
      if(reg->curr_variable->type == variable_type::R_VAL &&
      var.type == variable_type::R_VAL && reg->curr_variable->getValue() == var.getValue()) {
        chosen_reg = reg;
        break;
      } else if(reg->curr_variable->type == variable_type::VAR && var.type == variable_type::VAR &&
      reg->curr_variable->getVariableName() == var.getVariableName()) { // check r values
        chosen_reg = reg;
        break;
      } else if(reg->curr_variable->type == variable_type::ARR && var.type == variable_type::ARR &&
      reg->curr_variable->getVariableName() == var.getVariableName() &&
      reg->curr_variable->getValue() == var.getValue()) {
        chosen_reg = reg;
        break;
      }
    }
  }
  if(!chosen_reg) {
    if(accumulator_->curr_variable) {
      if(accumulator_->curr_variable->type == variable_type::R_VAL &&
          var.type == variable_type::R_VAL && accumulator_->curr_variable->getValue() == var.getValue()) {
        chosen_reg = accumulator_;
      } else if(accumulator_->curr_variable->type == variable_type::VAR && var.type == variable_type::VAR &&
          accumulator_->curr_variable->getVariableName() == var.getVariableName()) { // check r values
        chosen_reg = accumulator_;
      } else if(accumulator_->curr_variable->type == variable_type::ARR && var.type == variable_type::ARR &&
          accumulator_->curr_variable->getVariableName() == var.getVariableName() &&
          accumulator_->curr_variable->getValue() == var.getValue()) {
        chosen_reg = accumulator_;
      }
    }
  }
  return chosen_reg;
}

// TODO(Jakub Drzewiecki): Refactor needed, but accumulator is already free
std::shared_ptr<Register> FlowGraph::loadVariable(VariableContainer var, std::shared_ptr<GraphNode> node) {
  std::shared_ptr<Register> chosen_reg = checkVariableAlreadyLoaded(var);
  // TODO(Jakub Drzewiecki): Add handling process arguments variables
  if(!chosen_reg) {
    chosen_reg = findFreeRegister(node);
    std::shared_ptr<Register> empty_reg_for_index_var;
    chosen_reg->curr_variable = std::make_shared<VariableContainer>(var);
    chosen_reg->currently_used_ = true;
    unsigned long long int target_val = 0;
    bool is_procedure_argument = false;
    // algorithm for creating binary representation of mem address
    if(var.type == variable_type::R_VAL) {
      target_val = var.getValue();
    } else if(var.type == variable_type::VAR) {
      auto sym = current_symbol_table_->findSymbol(var.getVariableName());
      target_val = sym->mem_start;
    } else if(var.type == variable_type::ARR) {
      auto sym = current_symbol_table_->findSymbol(var.getVariableName());
      target_val = sym->mem_start + var.getValue();
    } else {
      auto sym = current_symbol_table_->findSymbol(var.getVariableName());
      target_val = sym->mem_start;
      auto index_sym = current_symbol_table_->findSymbol(var.getIndexVariableName());
      unsigned long long int index_var_target_val = index_sym->mem_start;
      empty_reg_for_index_var = findFreeRegister(node);
      empty_reg_for_index_var->currently_used_ = true;
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
        empty_reg_for_index_var->currently_used_ = false;
      }
      node->code_list_.push_back("LOAD " + chosen_reg->register_name_);
      node->code_list_.push_back("PUT " + chosen_reg->register_name_);
    }
  }
  return chosen_reg;
}

void FlowGraph::loadVariableToAccumulator(VariableContainer var, std::shared_ptr<GraphNode> node) {
  std::shared_ptr<Register> chosen_reg = checkVariableAlreadyLoaded(var);
  // TODO(Jakub Drzewiecki): Add handling process arguments variables

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
      auto sym = current_symbol_table_->findSymbol(var.getVariableName());
      target_val = sym->mem_start;
    } else if(var.type == variable_type::ARR) {
      auto sym = current_symbol_table_->findSymbol(var.getVariableName());
      target_val = sym->mem_start + var.getValue();
    } else {
      auto sym = current_symbol_table_->findSymbol(var.getVariableName());
      target_val = sym->mem_start;
      auto index_sym = current_symbol_table_->findSymbol(var.getIndexVariableName());
      unsigned long long int index_var_target_val = index_sym->mem_start;
      empty_reg_for_index_var = findFreeRegister(node);
      empty_reg_for_index_var->currently_used_ = true;
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
        empty_reg_for_index_var->currently_used_ = false;
      }
      node->code_list_.push_back("LOAD " + accumulator_->register_name_);
    }
  }
}

// TODO(Jakub Drzewiecki): Firstly check if both variables are not already loaded to accumulator or different register
void FlowGraph::handleAssignmentCommand(AssignmentCommand *command, std::shared_ptr<GraphNode> node) {
  std::vector<VariableContainer> needed_variables = command->expression_.neededVariablesInRegisters();
  // search if any of the variables are already loaded and reserve the registers
  bool last_var_loaded_in_acc = false;
  int loaded_vars = 0;
  std::vector<std::shared_ptr<Register>> searched_variables_in_regs;
  for(int i = 0; i < needed_variables.size(); i++) {
    auto var = needed_variables.at(i);
    std::shared_ptr<Register> searched_reg_var = checkVariableAlreadyLoaded(var);
    if(searched_reg_var) {
      loaded_vars++;
      searched_reg_var->currently_used_ = true;
      searched_variables_in_regs.push_back(searched_reg_var);
      if(searched_reg_var->register_name_ == "a") {
        if(i == needed_variables.size() - 1) {
          last_var_loaded_in_acc = true;
        }
      }
    }
  }
  if(!last_var_loaded_in_acc || loaded_vars != needed_variables.size() - 1) {
    moveAccumulatorToFreeRegister(node);
  }
  // store vals in registers in good order
  std::vector<std::shared_ptr<Register>> prepared_registers;
  for(int i = 0; i < needed_variables.size() - 1; i++) {
    prepared_registers.push_back(loadVariable(needed_variables.at(i), node));
  }
  // store accumulator variable last in vector
  loadVariableToAccumulator(needed_variables.at(needed_variables.size() - 1), node);
  prepared_registers.push_back(accumulator_);
  // TODO(Jakub Drzewiecki): Might need a way to handle adding jump commands
  std::vector<std::string> generated_commands =
      command->expression_.calculateExpression(prepared_registers);
  for(auto reg : searched_variables_in_regs) {
    reg->currently_used_ = false;
  }
  // save variable indexed arrays since such value cannot be kept in registers, no chance of saving them later
  // TODO(Jakub Drzewiecki): Finish here
  if(command->left_var_.type == variable_type::VARIABLE_INDEXED_ARR) {

  } else {
    accumulator_->curr_variable = std::make_shared<VariableContainer>(command->left_var_);
    accumulator_->variable_saved_ = false;
    accumulator_->currently_used_ = false;
  }
  for(auto generated_code : generated_commands) {
    node->code_list_.push_back(generated_code);
  }
  for(auto reg : prepared_registers) {
    reg->currently_used_ = false;
  }
}

void FlowGraph::handleReadCommand(ReadCommand *command, std::shared_ptr<GraphNode> node) {
  moveAccumulatorToFreeRegister(node);
  node->code_list_.push_back("READ");
  accumulator_->curr_variable = std::make_shared<VariableContainer>(command->var_);
}

void FlowGraph::handleWriteCommand(WriteCommand *command, std::shared_ptr<GraphNode> node) {
  std::shared_ptr<Register> reg = checkVariableAlreadyLoaded(command->written_value_);
  reg->currently_used_ = true;
  if(!reg) {
    moveAccumulatorToFreeRegister(node);
    loadVariableToAccumulator(command->written_value_, node);
  } else {
    if(reg->register_name_ != "a") {
      moveAccumulatorToFreeRegister(node);
      node->code_list_.push_back("GET " + reg->register_name_);
    }
  }
  node->code_list_.push_back("WRITE");
}
