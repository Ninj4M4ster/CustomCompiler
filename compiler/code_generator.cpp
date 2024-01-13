#include "code_generator.h"

CodeGenerator::CodeGenerator() {
  std::vector<std::string> register_names{"b", "c", "d", "e", "f", "g", "h"};
  accumulator_ = std::make_shared<Register>();
  accumulator_->register_name_ = "a";
  for(auto name : register_names) {
    std::shared_ptr<Register> new_reg = std::make_shared<Register>();
    new_reg->register_name_ = name;
    registers_.push_back(new_reg);
  }
}

void CodeGenerator::generateFlowGraph(Procedure main, std::vector<Procedure> procedures) {
  for(auto proc : procedures) {
    symbol_tables_.push_back(proc.symbol_table);
    procedures_start_nodes_.push_back(generateSingleFlowGraph(proc));
    procedures_names_.push_back(proc.head.name);
  }
  symbol_tables_.push_back(main.symbol_table);
  start_node = generateSingleFlowGraph(main);
}

// TODO(Jakub Drzewiecki): Reset registers after generating code for each procedure
void CodeGenerator::generateCode() {
  bool first_proc = true;
  for(int i = 0; i < procedures_start_nodes_.size(); i++) {
    current_symbol_table_ = symbol_tables_.at(i);
    auto proc_start = procedures_start_nodes_.at(i);
    if(first_proc) {
      current_start_line_ = 1;
      generate_jump_to_main_ = true;
      first_proc = false;
    }
    generateProcedureStart(proc_start);
    generateCodePreorder(proc_start);
    generateProcedureEnd(proc_start);
  }
  current_symbol_table_ = symbol_tables_.at(symbol_tables_.size() - 1);
  generateCodePreorder(start_node);
}

std::vector<std::shared_ptr<GraphNode>> CodeGenerator::getGraphs() {
  std::vector<std::shared_ptr<GraphNode>> nodes(procedures_start_nodes_);
  nodes.push_back(start_node);
  return nodes;
}

std::shared_ptr<GraphNode> CodeGenerator::generateSingleFlowGraph(Procedure proc) {
  std::shared_ptr<GraphNode> curr_node = std::make_shared<GraphNode>();
  curr_node->proc_name = proc.head.name;
  process_commands(proc.commands, curr_node);
  return curr_node;
}

void CodeGenerator::process_commands(std::vector<Command *> comms,
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
        curr_node->right_node = next_code;
        curr_node = next_code;
      }
    } else {
      curr_node->commands.push_back(comm);
    }
  }
}

void CodeGenerator::generateCodePreorder(std::shared_ptr<GraphNode> node) {
  long long int code_length_before_generation = node->code_list_.size();
  // generate code for current node
  if(code_length_before_generation == 0)
    node->start_line_ = current_start_line_;
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
      ProcedureCallCommand* procedure_call_command = static_cast<ProcedureCallCommand*>(comm);
      handleProcedureCallCommand(procedure_call_command, node);
    }
  }
  std::vector<std::shared_ptr<Register>> saved_regs;
  if(node->cond) {
    // save all registers and prepare condition
    saveRegistersValues(node);
    long long int code_length_after_generation = node->code_list_.size();
    current_start_line_ += code_length_after_generation - code_length_before_generation;
    node->condition_start_line_ = current_start_line_;
    std::cout << node->condition_start_line_ << std::endl;
    prepareCondition(node);
    long long int code_length_after_condition_preparation = node->code_list_.size();
    current_start_line_ += node->cond->getConditionCodeSize() +
        (code_length_after_condition_preparation - code_length_after_generation);
  } else {
    long long int code_length_after_generation = node->code_list_.size();
    current_start_line_ += (code_length_after_generation - code_length_before_generation);
  }
  if(node->jump_line_target) {  // save registers state after condition
    saved_regs = saveRegistersState();
  }

  // generate left node code
  if(node->left_node != nullptr) {
    generateCodePreorder(node->left_node);
    if(node->left_node->jump_line_target) {  // save variables from registers after ending THEN block
      std::shared_ptr<GraphNode> tmp_node = node->left_node;
      while(tmp_node->right_node) {
        tmp_node = tmp_node->right_node;
      }
      long long int code_length_bef_saving_variables = tmp_node->code_list_.size();
      moveAccumulatorToFreeRegister(tmp_node);
      saveRegistersValues(tmp_node);
      current_start_line_ += tmp_node->code_list_.size() - code_length_bef_saving_variables;
    }
  }
  if(node->cond) {
    generateCondition(node);
  }
  if(node->jump_line_target) {
    loadRegistersState(saved_regs);
  }
  if(node->right_node != nullptr) {
    generateCodePreorder(node->right_node);
    if(node->left_node && node->left_node->jump_line_target) {  // save variables from registers after ending ELSE block
      std::shared_ptr<GraphNode> tmp_node = node->right_node;
      while(tmp_node->right_node) {
        tmp_node = tmp_node->right_node;
      }
      long long int code_length_bef_saving_variables = tmp_node->code_list_.size();
      moveAccumulatorToFreeRegister(tmp_node);
      saveRegistersValues(tmp_node);
      current_start_line_ += tmp_node->code_list_.size() - code_length_bef_saving_variables;
    }
  }

  if(node->jump_line_target || node->jump_condition_target) {
    current_start_line_++;
  }
}

void CodeGenerator::generateProcedureStart(std::shared_ptr<GraphNode> node) {
  node->start_line_ = current_start_line_;
  // store register h to proper memory address
  auto sym = current_symbol_table_->getProcedureJumpBackMemoryAddressSymbol();
  auto reg = registers_.at(0);
  getValueIntoRegister(sym->mem_start, reg, node);
  node->code_list_.push_back("STORE " + reg->register_name_);
  current_start_line_ += node->code_list_.size();
  node->node_offset_ = current_start_line_ - 1;
}

void CodeGenerator::generateProcedureEnd(std::shared_ptr<GraphNode> node) {
  // travel to last node
  while(node->right_node) {
    node = node->right_node;
  }
  long long int code_size_before_generation = node->code_list_.size();
  // save all procedure arguments
  moveAccumulatorToFreeRegister(node);
  auto free_reg = findFreeRegister(node);
  for(auto reg : registers_) {
    if(reg->curr_variable &&
    (reg->curr_variable->type == variable_type::VAR || reg->curr_variable->type == variable_type::ARR)) {
      auto sym = current_symbol_table_->findSymbol(reg->curr_variable->getVariableName());
      if(sym->type == symbol_type::PROC_ARGUMENT || sym->type == symbol_type::PROC_ARRAY_ARGUMENT) {
        saveVariableFromRegister(reg, free_reg, node);
      }
    }
  }

  // load jump back address and jump
  auto sym = current_symbol_table_->getProcedureJumpBackMemoryAddressSymbol();
  auto reg = findFreeRegister(node);
  getValueIntoRegister(sym->mem_start, reg, node);
  node->code_list_.push_back("LOAD " + reg->register_name_);
  node->code_list_.push_back("INC a");
  node->code_list_.push_back("INC a");
  node->code_list_.push_back("JUMPR a");
  long long int code_size_after_generation = node->code_list_.size();
  current_start_line_ += (code_size_after_generation - code_size_before_generation);

  for(auto each_reg : registers_) {
    each_reg->curr_variable = nullptr;
    each_reg->variable_saved_ = true;
    each_reg->currently_used_ = false;
  }
  accumulator_->variable_saved_ = true;
  accumulator_->curr_variable = nullptr;
  accumulator_->currently_used_ = false;
}

// TODO(Jakub Drzewiecki): Possible missing handling which variable is in accumulator (could be other registers too)
void CodeGenerator::saveVariableFromRegister(std::shared_ptr<Register> reg,
                                         std::shared_ptr<Register> other_free_reg,
                                         std::shared_ptr<GraphNode> node) {
  if(reg->variable_saved_) {
    reg->curr_variable = nullptr;
    reg->currently_used_ = false;
    return;
  }
  auto var = reg->curr_variable;
  if(!var) {
    reg->curr_variable = nullptr;
    reg->currently_used_ = false;
    reg->variable_saved_ = true;
    return;
  }
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
  node->code_list_.push_back("PUT " + reg->register_name_);
  reg->variable_saved_ = true;
  for(auto other_reg : registers_) {
    if(other_reg->curr_variable && other_reg->register_name_ != reg->register_name_ &&
    other_reg->register_name_ != reg->register_name_ &&
    other_reg->curr_variable->type == reg->curr_variable->type &&
    other_reg->curr_variable->getVariableName() == reg->curr_variable->getVariableName() &&
    other_reg->curr_variable->getValue() == reg->curr_variable->getValue() &&
    other_reg->curr_variable->getIndexVariableName() == reg->curr_variable->getIndexVariableName()) {
      other_reg->variable_saved_ = true;
    }
  }
}

/**
 * This method moves variable from accumulator if the variable in accumulator is not saved.
 * In case there is only one register with saved/without value value left, one value from registers will be saved.
 * Currently_used flag is ignored.
 *
 * @param node
 */
// TODO(Jakub Drzewiecki): Consider overwriting register with old variable value instead of outputting to new register
// TODO(Jakub Drzewiecki): Don't move variable from accumulator if it is already stored in other register
void CodeGenerator::moveAccumulatorToFreeRegister(std::shared_ptr<GraphNode> node) {
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
 * @TODO(Jakub Drzewiecki): Target using registers that do not have procedures' arguments loaded
 */
std::shared_ptr<Register> CodeGenerator::findFreeRegister(std::shared_ptr<GraphNode> node) {
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

void CodeGenerator::getValueIntoRegister(long long value, std::shared_ptr<Register> reg, std::shared_ptr<GraphNode> node) {
  node->code_list_.push_back("RST " + reg->register_name_);
  if(value == 0)
    return;
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

std::shared_ptr<Register> CodeGenerator::checkVariableAlreadyLoaded(VariableContainer* var) {
  std::shared_ptr<Register> chosen_reg = nullptr;
  if(var->type == variable_type::VARIABLE_INDEXED_ARR)
    return chosen_reg;
  for(auto reg : registers_) {
    if(reg->curr_variable) {
      if(reg->curr_variable->type == variable_type::R_VAL &&
      var->type == variable_type::R_VAL && reg->curr_variable->getValue() == var->getValue()) {
        chosen_reg = reg;
        break;
      } else if(reg->curr_variable->type == variable_type::VAR && var->type == variable_type::VAR &&
      reg->curr_variable->getVariableName() == var->getVariableName()) { // check r values
        chosen_reg = reg;
        break;
      } else if(reg->curr_variable->type == variable_type::ARR && var->type == variable_type::ARR &&
      reg->curr_variable->getVariableName() == var->getVariableName() &&
      reg->curr_variable->getValue() == var->getValue()) {
        chosen_reg = reg;
        break;
      } else if(reg->curr_variable->type == variable_type::VARIABLE_INDEXED_ARR &&
          var->type == variable_type::VARIABLE_INDEXED_ARR &&
          reg->curr_variable->getVariableName() == var->getVariableName() &&
          reg->curr_variable->getIndexVariableName() == var->getIndexVariableName()) {
        chosen_reg = reg;
        break;
      }
    }
  }
  if(!chosen_reg) {
    if(accumulator_->curr_variable) {
      if(accumulator_->curr_variable->type == variable_type::R_VAL &&
          var->type == variable_type::R_VAL && accumulator_->curr_variable->getValue() == var->getValue()) {
        chosen_reg = accumulator_;
      } else if(accumulator_->curr_variable->type == variable_type::VAR && var->type == variable_type::VAR &&
          accumulator_->curr_variable->getVariableName() == var->getVariableName()) { // check r values
        chosen_reg = accumulator_;
      } else if(accumulator_->curr_variable->type == variable_type::ARR && var->type == variable_type::ARR &&
          accumulator_->curr_variable->getVariableName() == var->getVariableName() &&
          accumulator_->curr_variable->getValue() == var->getValue()) {
        chosen_reg = accumulator_;
      } else if(accumulator_->curr_variable->type == variable_type::VARIABLE_INDEXED_ARR &&
          var->type == variable_type::VARIABLE_INDEXED_ARR &&
          accumulator_->curr_variable->getVariableName() == var->getVariableName() &&
          accumulator_->curr_variable->getIndexVariableName() == var->getIndexVariableName()) {
        chosen_reg = accumulator_;
      }
    }
  }
  return chosen_reg;
}

// TODO(Jakub Drzewiecki): Some refactor can still be done, but it is a job for later.
//  Current version of this function should work properly
std::shared_ptr<Register> CodeGenerator::loadVariable(VariableContainer* var,
                                                      std::shared_ptr<Register> target_reg,
                                                      std::shared_ptr<GraphNode> node,
                                                      bool use_saved_variables) {
  std::shared_ptr<Register> reg_with_loaded_var = checkVariableAlreadyLoaded(var);
  if(reg_with_loaded_var) {  // variable found, load it to proper register if needed
    if(target_reg) {
      if(reg_with_loaded_var->register_name_ != target_reg->register_name_) {
        if(reg_with_loaded_var->register_name_ != "a") {
          node->code_list_.push_back("GET " + reg_with_loaded_var->register_name_);
        }
        if(target_reg->register_name_ != "a") {
          node->code_list_.push_back("PUT " + target_reg->register_name_);
        }
        target_reg->variable_saved_ = reg_with_loaded_var->variable_saved_;
        target_reg->curr_variable = reg_with_loaded_var->curr_variable;
      }
    } else {
      if(reg_with_loaded_var->register_name_ == "a") {
        target_reg = findFreeRegister(node);
        target_reg->variable_saved_ = reg_with_loaded_var->variable_saved_;
        target_reg->curr_variable = reg_with_loaded_var->curr_variable;
        node->code_list_.push_back("PUT " + target_reg->register_name_);
      } else {
        target_reg = reg_with_loaded_var;
      }
    }
  } else {  // variable needs to be loaded
    if(!target_reg) {
      target_reg = findFreeRegister(node);
      target_reg->currently_used_ = true;
    }
    if(var->type == variable_type::R_VAL) {
      getValueIntoRegister(var->getValue(), target_reg, node);
    } else if(var->type == variable_type::VAR) {
      auto sym = current_symbol_table_->findSymbol(var->getVariableName());
      getValueIntoRegister(sym->mem_start, target_reg, node);
      node->code_list_.push_back("LOAD " + target_reg->register_name_);
      if(sym->type == symbol_type::PROC_ARGUMENT) {  // load again, because variable address is stored in memory
        node->code_list_.push_back("LOAD " + accumulator_->register_name_);
      }
      if(target_reg->register_name_ != "a")
        node->code_list_.push_back("PUT " + target_reg->register_name_);
    } else if(var->type == variable_type::ARR) {
      auto sym = current_symbol_table_->findSymbol(var->getVariableName());
      if(sym->type == symbol_type::ARR) {
        getValueIntoRegister(sym->mem_start + var->getValue(), target_reg, node);
        node->code_list_.push_back("LOAD " + target_reg->register_name_);
        if(target_reg->register_name_ != "a")
          node->code_list_.push_back("PUT " + target_reg->register_name_);
      } else {  // array is a procedure argument
        getValueIntoRegister(sym->mem_start, target_reg, node);
        node->code_list_.push_back("LOAD " + target_reg->register_name_);
        getValueIntoRegister(var->getValue(), target_reg, node);
        node->code_list_.push_back("ADD " + target_reg->register_name_);
        node->code_list_.push_back("LOAD " + accumulator_->register_name_);
        if(target_reg->register_name_ != "a")
          node->code_list_.push_back("PUT " + target_reg->register_name_);
      }
    } else if(var->type == variable_type::VARIABLE_INDEXED_ARR) {
      auto sym = current_symbol_table_->findSymbol(var->getVariableName());
      // firstly load index variable
      Variable* index_var = new Variable;
      index_var->type = variable_type::VAR;
      index_var->var_name = var->getIndexVariableName();
      target_reg->currently_used_ = false;
      target_reg = checkVariableAlreadyLoaded(index_var);
      if(!target_reg) {
        target_reg = findFreeRegister(node);
        auto index_var_sym = current_symbol_table_->findSymbol(index_var->getVariableName());
        getValueIntoRegister(index_var_sym->mem_start, target_reg, node);
        node->code_list_.push_back("LOAD " + target_reg->register_name_);
        if(index_var_sym->type == symbol_type::PROC_ARGUMENT) {
          node->code_list_.push_back("LOAD " + accumulator_->register_name_);
        }
        node->code_list_.push_back("PUT " + target_reg->register_name_);
      } else if(target_reg->register_name_ == "a") {
        std::shared_ptr<Register> tmp_reg = findFreeRegister(node);
        node->code_list_.push_back("PUT " + tmp_reg->register_name_);
        tmp_reg->curr_variable = target_reg->curr_variable;
        tmp_reg->variable_saved_ = target_reg->variable_saved_;
        tmp_reg->currently_used_ = true;
        target_reg->curr_variable = nullptr;
        target_reg->variable_saved_ = true;
        target_reg->currently_used_ = false;
        target_reg = tmp_reg;
      }
      // then load target array variable
      if(sym->type == symbol_type::ARR) {  // normal array
        getValueIntoRegister(sym->mem_start, accumulator_, node);
        node->code_list_.push_back("ADD " + target_reg->register_name_);
        node->code_list_.push_back("LOAD " + accumulator_->register_name_);
        if(target_reg->register_name_ != "a")
          node->code_list_.push_back("PUT " + target_reg->register_name_);
        target_reg->currently_used_ = false;
      } else {  // procedure argument array
        getValueIntoRegister(sym->mem_start, accumulator_, node);
        node->code_list_.push_back("LOAD " + accumulator_->register_name_);
        node->code_list_.push_back("ADD " + target_reg->register_name_);
        node->code_list_.push_back("LOAD " + accumulator_->register_name_);
        if(target_reg->register_name_ != "a")
          node->code_list_.push_back("PUT " + target_reg->register_name_);
        target_reg->currently_used_ = false;
      }
    }
    target_reg->curr_variable = var;
    target_reg->currently_used_ = false;
    target_reg->variable_saved_ = true;
  }
  if(target_reg->curr_variable && !target_reg->variable_saved_) {
    target_reg->currently_used_ = true;
    std::shared_ptr<Register> free_one = findFreeRegister(node);
    free_one->currently_used_ = true;
    saveVariableFromRegister(target_reg, free_one, node);
    target_reg->currently_used_ = false;
    free_one->currently_used_ = false;
  }
  return target_reg;
}

std::vector<std::shared_ptr<Register>> CodeGenerator::saveRegistersState() {
  std::vector<std::shared_ptr<Register>> registers_states;
  std::shared_ptr<Register> reg = std::make_shared<Register>(*accumulator_.get());
  registers_states.push_back(reg);
  for(auto curr_reg : registers_) {
    reg = std::make_shared<Register>(*curr_reg.get());
    registers_states.push_back(reg);
  }
  return registers_states;
}

void CodeGenerator::loadRegistersState(std::vector<std::shared_ptr<Register>> saved_regs) {
  accumulator_ = saved_regs.at(0);
  for(int i = 1; i < saved_regs.size(); i++) {
    registers_.at(i-1) = saved_regs.at(i);
  }
}

void CodeGenerator::saveRegistersValues(std::shared_ptr<GraphNode> node) {
  moveAccumulatorToFreeRegister(node);
  accumulator_->curr_variable = nullptr;
  accumulator_->variable_saved_ = true;
  std::shared_ptr<Register> free_reg = findFreeRegister(node);
  free_reg->currently_used_ = true;
  for(auto reg : registers_) {
    if(reg->register_name_ != free_reg->register_name_) {
      saveVariableFromRegister(reg, free_reg, node);
      reg->curr_variable = nullptr;
      reg->variable_saved_ = true;
    }
  }
  free_reg->currently_used_ = false;
}

void CodeGenerator::handleAssignmentCommand(AssignmentCommand *command, std::shared_ptr<GraphNode> node) {
  std::vector<VariableContainer*> needed_variables = command->expression_->neededVariablesInRegisters();
  // search if any of the variables are already loaded and reserve the registers
  std::vector<std::shared_ptr<Register>> searched_variables_in_regs;
  for(int i = 0; i < needed_variables.size(); i++) {
    auto var = needed_variables.at(i);
    std::shared_ptr<Register> searched_reg_var = checkVariableAlreadyLoaded(var);
    if(searched_reg_var) {
      searched_reg_var->currently_used_ = true;
      searched_variables_in_regs.push_back(searched_reg_var);
    }
  }
  moveAccumulatorToFreeRegister(node);

  // store vals in registers in good order
  // TODO(Jakub Drzewiecki): Save variables before expressions that needs them saved
  std::vector<std::shared_ptr<Register>> prepared_registers;
  bool use_saved_variables = true;
  for(int i = 0; i < needed_variables.size() - 1; i++) {
    std::shared_ptr<Register> reg_with_var = loadVariable(needed_variables.at(i), nullptr, node, use_saved_variables);
    reg_with_var->currently_used_ = true;
    prepared_registers.push_back(reg_with_var);
  }
  // store accumulator variable last in vector
  // TODO(Jakub Drzewiecki): Consider loading variable to different register, not only accumulator
  loadVariable(needed_variables.at(needed_variables.size() - 1), accumulator_, node, use_saved_variables);
  prepared_registers.push_back(accumulator_);
  for(int i = 0; i < command->expression_->neededEmptyRegs(); i++) {
    std::shared_ptr<Register> free_reg = findFreeRegister(node);
    free_reg->currently_used_ = true;
    prepared_registers.push_back(free_reg);
  }
  std::vector<std::string> generated_commands =
      command->expression_->calculateExpression(prepared_registers,
                                               current_start_line_ + node->code_list_.size() - node->node_offset_);
  for(auto reg : searched_variables_in_regs) {
    reg->currently_used_ = false;
  }
  for(auto reg : prepared_registers) {
    reg->currently_used_ = false;
  }
  for(auto generated_code : generated_commands) {
    node->code_list_.push_back(generated_code);
  }
  // save variable indexed arrays since such value cannot be kept in registers, no chance of saving them later
  if(command->left_var_->type == variable_type::VARIABLE_INDEXED_ARR) {
    // TODO(Jakub Drzewiecki): New flag not to delete value that is currently in saving process
    std::shared_ptr<Register> free_reg = findFreeRegister(node);
    free_reg->currently_used_ = true;
    node->code_list_.push_back("PUT " + free_reg->register_name_);
    free_reg->variable_saved_ = false;
    free_reg->curr_variable = command->left_var_;
    auto sym = current_symbol_table_->findSymbol(command->left_var_->getIndexVariableName());
    Variable* var = new variable;
    var->type = variable_type::VAR;
    var->var_name = command->left_var_->getIndexVariableName();
    auto var_ind_sym =
        current_symbol_table_->findSymbol(command->left_var_->getIndexVariableName());
    std::shared_ptr<Register> another_free_reg = checkVariableAlreadyLoaded(var);
    if(!another_free_reg) {
      another_free_reg = loadVariable(var, nullptr, node, false);  // process arguments variables should be already handled
    }
    getValueIntoRegister(sym->mem_start, accumulator_, node);
    node->code_list_.push_back("LOAD " + accumulator_->register_name_);
    if(sym->type == symbol_type::PROC_ARRAY_ARGUMENT) {
      node->code_list_.push_back("LOAD " + accumulator_->register_name_);
    }
    node->code_list_.push_back("ADD " + another_free_reg->register_name_);
    node->code_list_.push_back("PUT " + another_free_reg->register_name_);
    node->code_list_.push_back("GET " + free_reg->register_name_);
    node->code_list_.push_back("STORE " + another_free_reg->register_name_);
    free_reg->currently_used_ = false;
    free_reg->variable_saved_ = true;
    accumulator_->curr_variable = command->left_var_;
    accumulator_->currently_used_ = false;
    accumulator_->variable_saved_ = true;
    another_free_reg->currently_used_ = false;
    another_free_reg->curr_variable = nullptr;
    another_free_reg->variable_saved_ = true;
  } else {
    if(command->left_var_->type != variable_type::R_VAL) {
      for(auto reg : registers_) {
        if(reg->curr_variable && reg->curr_variable->type == command->left_var_->type &&
        reg->curr_variable->getVariableName() == command->left_var_->getVariableName() &&
        reg->curr_variable->getValue() == command->left_var_->getValue() &&
        reg->curr_variable->getIndexVariableName() == command->left_var_->getIndexVariableName()) {
          reg->curr_variable = nullptr;
          reg->variable_saved_ = true;
          reg->currently_used_ = false;
        }
      }
    }
    if(command->left_var_->type == variable_type::VAR) {
      for (auto reg : registers_) {
        if(reg->curr_variable && reg->curr_variable->type == variable_type::VARIABLE_INDEXED_ARR &&
        reg->curr_variable->getIndexVariableName() == command->left_var_->getVariableName()) {
          reg->curr_variable = nullptr;
          reg->variable_saved_ = true;
        }
      }
    }
    accumulator_->curr_variable = command->left_var_;
    accumulator_->variable_saved_ = false;
    accumulator_->currently_used_ = false;
  }
}

void CodeGenerator::handleReadCommand(ReadCommand *command, std::shared_ptr<GraphNode> node) {
  moveAccumulatorToFreeRegister(node);
  node->code_list_.push_back("READ");
  accumulator_->curr_variable = command->var_;
  accumulator_->variable_saved_ = false;
}

void CodeGenerator::handleWriteCommand(WriteCommand *command, std::shared_ptr<GraphNode> node) {
  std::shared_ptr<Register> reg = checkVariableAlreadyLoaded(command->written_value_);
  if(!reg) {
    moveAccumulatorToFreeRegister(node);
    loadVariable(command->written_value_, accumulator_, node, false);
  } else {
    reg->currently_used_ = true;
    if(reg->register_name_ != "a") {
      moveAccumulatorToFreeRegister(node);
      node->code_list_.push_back("GET " + reg->register_name_);
      accumulator_->curr_variable = reg->curr_variable;
    }
  }
  node->code_list_.push_back("WRITE");
}

// TODO(Jakub Drzewiecki): For procedures with <= 3 arguments,
//  variables can be passed in registers instead of memory addresses.
//  Consider also passing memory addresses of arguments in registers instead of saving it,
//  this can allow for up to 6 arguments being passed in registers.
void CodeGenerator::handleProcedureCallCommand(ProcedureCallCommand *command,
                                               std::shared_ptr<GraphNode> node) {
  // save all currently unsaved variables
  moveAccumulatorToFreeRegister(node);
  std::shared_ptr<Register> free_reg = findFreeRegister(node);
  for(auto reg : registers_) {
    if(reg->register_name_ != free_reg->register_name_) {
      saveVariableFromRegister(reg, free_reg, node);
    }
  }
//  node->code_list_.push_back("# vars saved ");
  // pass variables memory addresses to procedures memory reserved for declared arguments
  for(auto arg : command->proc_call_.args) {
    auto sym = current_symbol_table_->findSymbol(arg.name);
    auto target_sym = arg.target_variable_symbol;
    getValueIntoRegister(sym->mem_start, free_reg, node);
    node->code_list_.push_back("GET " + free_reg->register_name_);
    if(sym->type == symbol_type::PROC_ARGUMENT || sym->type == symbol_type::PROC_ARRAY_ARGUMENT) {
      node->code_list_.push_back("LOAD " + accumulator_->register_name_);
    }
    getValueIntoRegister(target_sym->mem_start, free_reg, node);
    node->code_list_.push_back("STORE " + free_reg->register_name_);
  }

//  node->code_list_.push_back("# addresses stored");

  // pass current line number in register h
  node->code_list_.push_back("STRK a");
  // add procedure call
  int i = 0;
  for(; i < procedures_names_.size(); i++) {
    if(procedures_names_.at(i) == command->proc_call_.name) {
      break;
    }
  }
  auto proc_node_start = procedures_start_nodes_.at(i);
  node->code_list_.push_back("JUMP " + std::to_string(proc_node_start->start_line_));
}

void CodeGenerator::prepareCondition(std::shared_ptr<GraphNode> node) {
  std::vector<VariableContainer*> needed_variables = node->cond->neededVariablesInRegisters();
  // search if any of the variables are already loaded and reserve the registers
  std::vector<std::shared_ptr<Register>> searched_variables_in_regs;
  for(int i = 0; i < needed_variables.size(); i++) {
    auto var = needed_variables.at(i);
    std::shared_ptr<Register> searched_reg_var = checkVariableAlreadyLoaded(var);
    if(searched_reg_var) {
      searched_reg_var->currently_used_ = true;
      searched_variables_in_regs.push_back(searched_reg_var);
      std::cout << "Var found " << searched_reg_var->curr_variable->stringify() << " in reg " << searched_reg_var->register_name_ << std::endl;
    }
  }
  moveAccumulatorToFreeRegister(node);

  // store vals in registers in good order
  std::vector<std::shared_ptr<Register>> prepared_registers;
  bool use_saved_variables = true;
  for(int i = 0; i < needed_variables.size() - 1; i++) {
    prepared_registers.push_back(loadVariable(needed_variables.at(i), nullptr, node, use_saved_variables));
  }
  // store accumulator variable last in vector
  loadVariable(needed_variables.at(needed_variables.size() - 1), accumulator_, node, use_saved_variables);
  prepared_registers.push_back(accumulator_);
  std::shared_ptr<Register> free_reg = findFreeRegister(node);
  prepared_registers.push_back(free_reg);
  // save registers prepared for condition
  for(auto prep_reg : prepared_registers) {
    node->regs_prepared_for_condition.push_back(std::make_shared<Register>(*prep_reg.get()));
  }

  node->cond->updateRegistersState(prepared_registers);

  for(auto reg : searched_variables_in_regs) {
    reg->currently_used_ = false;
  }
  for(auto reg : prepared_registers) {
    reg->currently_used_ = false;
  }
}

void CodeGenerator::generateCondition(std::shared_ptr<GraphNode> node) {
  std::vector<std::shared_ptr<Register>> prepared_registers;
  for(auto prep_reg : node->regs_prepared_for_condition) {
    prepared_registers.push_back(std::make_shared<Register>(*prep_reg.get()));
  }
  node->regs_prepared_for_condition.clear();

  std::vector<std::string> generated_code =
      node->cond->generateCondition(prepared_registers,
                                    node->left_node->start_line_,
                                    current_start_line_);

  for(auto gen_command : generated_code) {
    node->code_list_.push_back(gen_command);
  }
}