#include <fstream>
#include "compiler.h"

Compiler::Compiler() {
  current_symbol_table_ = std::make_shared<SymbolTable>();
  code_generator_ = std::make_shared<CodeGenerator>();
}

void Compiler::setOutputFileName(std::string f_name) {
  output_file_name_ = f_name;
}

void Compiler::compile() {
  code_generator_->generateFlowGraph(main_, procedures_);
  code_generator_->generateCode();
  std::vector<std::shared_ptr<GraphNode>> graphs_start_nodes = code_generator_->getGraphs();
  std::cout << "jest kod\n";
  outputCode(graphs_start_nodes);
}

void Compiler::declareProcedure(std::vector<Command*> commands) {
  Procedure proc;
  proc.head = curr_procedure_head_;
  curr_procedure_head_ = ProcedureHead{};
  proc.commands = commands;
  proc.symbol_table = current_symbol_table_;
  current_symbol_table_ = std::make_shared<SymbolTable>();
  procedures_.push_back(proc);
}

void Compiler::declareMain(std::vector<Command*> commands) {
  Procedure proc;
  proc.symbol_table = current_symbol_table_;
  current_symbol_table_ = std::make_shared<SymbolTable>();
  proc.commands = commands;
  main_ = proc;
}

void Compiler::declareProcedureHead(std::string procedure_name, int line_number) {
  for(auto & proc : procedures_) {  // check if procedure name is available
    if(proc.head.name == procedure_name) {
      throw std::runtime_error("Error at line " + std::to_string(line_number)
      + ": multiple declarations of procedure \"" + procedure_name + "\".");
    }
  }
  curr_procedure_head_.name = procedure_name;
  curr_procedure_head_.arguments = current_procedure_arguments_;
  current_procedure_arguments_.clear();
  Symbol new_symbol;
  setSymbolBounds(new_symbol, 1, line_number);
  current_symbol_table_->addProcJumpBackMemoryAddress(new_symbol);
}

void Compiler::declareVariable(std::string variable_name, int line_number) {
  Symbol new_symbol = createSymbol(variable_name, symbol_type::VAR);
  setSymbolBounds(new_symbol, 1, line_number);
  current_symbol_table_->addSymbol(new_symbol, line_number);
}

void Compiler::declareVariable(std::string variable_name, long long array_size, int line_number) {
  Symbol new_symbol = createSymbol(variable_name, symbol_type::ARR);
  setSymbolBounds(new_symbol, array_size, line_number);
  current_symbol_table_->addSymbol(new_symbol, line_number);
}

void Compiler::declareProcedureArgument(std::string variable_name, int line_number) {
  Symbol new_symbol = createSymbol(variable_name, symbol_type::PROC_ARGUMENT);
  setSymbolBounds(new_symbol, 1, line_number);
  current_symbol_table_->addSymbol(new_symbol, line_number);
  current_procedure_arguments_.push_back({variable_name, symbol_type::VAR});
}

void Compiler::declareProcedureArrayArgument(std::string variable_name, int line_number) {
  Symbol new_symbol = createSymbol(variable_name, symbol_type::PROC_ARRAY_ARGUMENT);
  setSymbolBounds(new_symbol, 1, line_number);
  current_symbol_table_->addSymbol(new_symbol, line_number);
  current_procedure_arguments_.push_back({variable_name, symbol_type::ARR});
}

void Compiler::addProcedureCallArgument(std::string variable_name, int line_number) {
  std::shared_ptr<Symbol> sym = current_symbol_table_->findSymbol(variable_name);
  if(sym == nullptr) {  // no symbol found
    throw std::runtime_error("Error at line " + std::to_string(line_number) + ": unknown variable name.");
  }
  ProcedureCallArgument arg;
  arg.name = variable_name;
  arg.type = sym->type;
  arg.needs_initialization_before_call = sym->initialized;
  current_procedure_call_arguments_.push_back(arg);
}

void Compiler::createProcedureCall(std::string procedure_name, int line_number) {
  if(procedure_name == curr_procedure_head_.name) {
    throw std::runtime_error("Error at line " + std::to_string(line_number) +
    ": recursive procedure call is not allowed.");
  }
  // check if called procedure is already declared
  bool proc_found = false;
  Procedure called_procedure;
  for(auto & proc : procedures_) {
    if(proc.head.name == procedure_name) {
      proc_found = true;
      called_procedure = proc;
    }
  }
  if(!proc_found) {
    throw std::runtime_error("Error at line " + std::to_string(line_number) +
    ": procedure " + procedure_name + " is not declared.");
  }
  // check arguments of called procedure
  if(current_procedure_call_arguments_.size() != called_procedure.head.arguments.size()) {
    throw std::runtime_error("Error at line " + std::to_string(line_number) +
        ": procedure " + procedure_name + " expects " + std::to_string(called_procedure.head.arguments.size()) +
        " arguments, but " + std::to_string(current_procedure_call_arguments_.size()) + " were passed.");
  }
  for(int i = 0; i < current_procedure_call_arguments_.size(); i++) {
    auto proc_arg_type = current_procedure_call_arguments_.at(i).type;
    auto proc_decl_arg_type = called_procedure.head.arguments.at(i).type;
    bool arg_initialization_flag = current_procedure_call_arguments_.at(i).needs_initialization_before_call;
    bool decl_arg_initialization_flag = called_procedure.head.arguments.at(i).needs_initialization_before_call;
    auto proc_arg_sym = current_symbol_table_->findSymbol(current_procedure_call_arguments_.at(i).name);
    auto proc_decl_arg_sym = called_procedure.symbol_table->findSymbol(called_procedure.head.arguments.at(i).name);
    if(proc_decl_arg_type == symbol_type::VAR &&
        (proc_arg_type == symbol_type::ARR || proc_arg_type == symbol_type::PROC_ARRAY_ARGUMENT)) {
      throw std::runtime_error("Error at line " + std::to_string(line_number) +
          ": procedure " + procedure_name + " expects variable, but array argument was passed.");
    }
    if(proc_decl_arg_type == symbol_type::ARR &&
        (proc_arg_type == symbol_type::VAR || proc_arg_type == symbol_type::PROC_ARGUMENT)) {
      throw std::runtime_error("Error at line " + std::to_string(line_number) +
          ": procedure " + procedure_name + " expects array, but variable argument was passed.");
    }
    if(proc_arg_type == symbol_type::VAR && decl_arg_initialization_flag && !arg_initialization_flag) {
      throw std::runtime_error("Error at line " + std::to_string(line_number) +
          ": procedure " + procedure_name + " expects initialized argument (" +
          called_procedure.head.arguments.at(i).name + "), but uninitialized variable has been given.");
    }
    if(proc_arg_type == symbol_type::PROC_ARGUMENT && decl_arg_initialization_flag && !arg_initialization_flag) {
      current_procedure_call_arguments_.at(i).needs_initialization_before_call = true;
    }
    if(proc_decl_arg_sym->initialized) {
      proc_arg_sym->initialized = true;
    }
    current_procedure_call_arguments_.at(i).target_variable_symbol = proc_decl_arg_sym;
  }
  ProcedureCall called_proc;
  called_proc.name = procedure_name;
  called_proc.args = current_procedure_call_arguments_;
  current_procedure_call_arguments_.clear();
  current_procedure_call_ = called_proc;
}

Command *Compiler::createAssignmentCommand(VariableContainer *left_var, DefaultExpression *expr, int line_number) {
  auto sym = current_symbol_table_->findSymbol(left_var->getVariableName());
  if(sym->type == symbol_type::PROC_ARGUMENT && !isProcedureArgumentMarked(left_var->getVariableName())) {
    sym->initialized = true;
  } else if(sym->type == symbol_type::VAR) {
    sym->initialized = true;
  }
  AssignmentCommand* comm = new AssignmentCommand;
  comm->expression_ = *expr;
  comm->left_var_ = left_var;
  comm->type = command_type::ASSIGNMENT;
  return comm;
}

Command *Compiler::createIfThenElseBlock(Condition *cond,
                                         std::vector<Command *> then_commands,
                                         std::vector<Command *> else_commands,
                                         int line_number) {
  IfElseCommand* comm = new IfElseCommand;
  comm->cond_ = *cond;
  comm->then_commands_ = then_commands;
  comm->else_commands_ = else_commands;
  comm->type = command_type::IF_ELSE;
  return comm;
}

Command *Compiler::createIfThenElseBlock(Condition *cond, std::vector<Command *> then_commands, int line_number) {
  IfElseCommand* comm = new IfElseCommand;
  comm->cond_ = *cond;
  comm->then_commands_ = then_commands;
  comm->type = command_type::IF_ELSE;
  return comm;
}

Command *Compiler::createWhileBlock(Condition *cond, std::vector<Command *> commands, int line_number) {
  WhileCommand* comm = new WhileCommand;
  comm->cond_ = *cond;
  comm->commands_ = commands;
  comm->type = command_type::WHILE;
  return comm;
}

Command *Compiler::createRepeatUntilBlock(Condition *cond, std::vector<Command *> commands, int line_number) {
  RepeatUntilCommand* comm = new RepeatUntilCommand;
  comm->cond_ = *cond;
  comm->commands_ = commands;
  comm->type = command_type::REPEAT;
  return comm;
}

Command *Compiler::createProcedureCallCommand(int line_number) {
  ProcedureCallCommand* comm = new ProcedureCallCommand;
  comm->proc_call_ = current_procedure_call_;
  comm->type = command_type::PROC_CALL;
  return comm;
}

Command *Compiler::createReadCommand(VariableContainer *var, int line_number) {
  auto sym = current_symbol_table_->findSymbol(var->getVariableName());
  if(sym->type == symbol_type::PROC_ARGUMENT && !isProcedureArgumentMarked(var->getVariableName())) {
    sym->initialized = true;
  } else if(sym->type == symbol_type::VAR) {
    sym->initialized = true;
  }
  ReadCommand* comm = new ReadCommand;
  comm->var_ = var;
  comm->type = command_type::READ;
  return comm;
}

Command *Compiler::createWriteCommand(VariableContainer *var, int line_number) {
  WriteCommand* comm = new WriteCommand;
  comm->written_value_ = var;
  comm->type = command_type::WRITE;
  return comm;
}

DefaultExpression *Compiler::createDefaultExpression(VariableContainer *var, int line_number) {
  DefaultExpression* expr = new DefaultExpression;
  expr->var_ = var;
  return expr;
}

DefaultExpression *Compiler::createPlusExpression(VariableContainer *left_var,
                                                  VariableContainer *right_var,
                                                  int line_number) {
  PlusExpression* expr = new PlusExpression;
  expr->var_ = left_var;
  expr->right_var_ = right_var;
  return expr;
}

DefaultExpression *Compiler::createMinusExpression(VariableContainer *left_var,
                                                   VariableContainer *right_var,
                                                   int line_number) {
  MinusExpression* expr = new MinusExpression;
  expr->var_ = left_var;
  expr->right_var_ = right_var;
  return expr;
}

DefaultExpression *Compiler::createMultiplyExpression(VariableContainer *left_var,
                                                      VariableContainer *right_var,
                                                      int line_number) {
  MultiplyExpression* expr = new MultiplyExpression;
  expr->var_ = left_var;
  expr->right_var_ = right_var;
  return expr;
}

DefaultExpression *Compiler::createDivideExpression(VariableContainer *left_var,
                                                    VariableContainer *right_var,
                                                    int line_number) {
  DivideExpression* expr = new DivideExpression;
  expr->var_ = left_var;
  expr->right_var_ = right_var;
  return expr;
}

DefaultExpression *Compiler::createModuloExpression(VariableContainer *left_var,
                                                    VariableContainer *right_var,
                                                    int line_number) {
  ModuloExpression* expr = new ModuloExpression;
  expr->var_ = left_var;
  expr->right_var_ = right_var;
  return expr;
}

Condition *Compiler::createEqualCondition(VariableContainer *left_var, VariableContainer *right_var, int line_number) {
  Condition* con = new Condition;
  con->type_ = condition_type::EQ;
  con->left_var_ = left_var;
  con->right_var_ = right_var;
  return con;
}

Condition *Compiler::createNotEqualCondition(VariableContainer *left_var, VariableContainer *right_var, int line_number) {
  Condition* con = new Condition;
  con->type_ = condition_type::NEQ;
  con->left_var_ = left_var;
  con->right_var_ = right_var;
  return con;
}

Condition *Compiler::createGreaterCondition(VariableContainer *left_var, VariableContainer *right_var, int line_number) {
  Condition* con = new Condition;
  con->type_ = condition_type::GT;
  con->left_var_ = left_var;
  con->right_var_ = right_var;
  return con;
}

Condition *Compiler::createGreaterEqualCondition(VariableContainer *left_var, VariableContainer *right_var, int line_number) {
  Condition* con = new Condition;
  con->type_ = condition_type::GE;
  con->left_var_ = left_var;
  con->right_var_ = right_var;
  return con;
}

VariableContainer* Compiler::getVariable(long long value, int line_number) {
  RValue *r_value_var = new RValue;
  r_value_var->type = variable_type::R_VAL;
  r_value_var->value = value;
  return r_value_var;
}

VariableContainer *Compiler::getVariable(std::string variable_name, int line_number) {
  std::shared_ptr<Symbol> sym = current_symbol_table_->findSymbol(variable_name);
  if(sym == nullptr) {  // no variable with given name found
    throw std::runtime_error("Error at line " + std::to_string(line_number) + ": unknown variable name.");
  }
  if(sym->type == symbol_type::ARR ||
  sym->type == symbol_type::PROC_ARRAY_ARGUMENT) {  // array passed without index argument
    throw std::runtime_error("Error at line " + std::to_string(line_number) + ": variable "
                                 + variable_name + " is of array type, but no index has been given.");
  }
  Variable * var = new Variable;
  var->type = variable_type::VAR;
  var->var_name = variable_name;
  return var;
}

VariableContainer *Compiler::getVariable(std::string variable_name, long long index, int line_number) {
  std::shared_ptr<Symbol> sym = current_symbol_table_->findSymbol(variable_name);
  if(sym == nullptr) {  // no variable with given name found
    throw std::runtime_error("Error at line " + std::to_string(line_number) + ": unknown variable name.");
  }
  if(sym->type == symbol_type::VAR ||
  sym->type == symbol_type::PROC_ARGUMENT) {  // normal variable accessed as array
    throw std::runtime_error("Error at line " + std::to_string(line_number) + ": variable "
                                 + variable_name + " of decimal type accessed array-like.");
  }
  if(sym->type == symbol_type::ARR && (sym->length <= index || index < 0)) {
    throw std::runtime_error("Error at line " + std::to_string(line_number) + ": array index out of bounds.");
  }
  // TODO(Jakub Drzewiecki): Add checking procedure array arguments bounds
  Array * arr = new Array;
  arr->type = variable_type::ARR;
  arr->var_name = variable_name;
  arr->index = index;
  return arr;
}

VariableContainer *Compiler::getVariable(std::string variable_name, std::string index_variable_name, int line_number) {
  std::shared_ptr<Symbol> sym = current_symbol_table_->findSymbol(variable_name);
  if(sym == nullptr) {  // no variable with given name found
    throw std::runtime_error("Error at line " + std::to_string(line_number) + ": unknown variable name - " + variable_name);
  }
  if(sym->type == symbol_type::VAR ||
      sym->type == symbol_type::PROC_ARGUMENT) {  // normal variable accessed as array
    throw std::runtime_error("Error at line " + std::to_string(line_number) + ": variable "
                                 + variable_name + " of decimal type accessed array-like.");
  }
  // check var_index symbol
  sym = current_symbol_table_->findSymbol(index_variable_name);
  if(sym == nullptr) {  // no variable with given name found
    throw std::runtime_error("Error at line " + std::to_string(line_number) + ": unknown variable name - " + index_variable_name);
  }
  if(sym->type == symbol_type::ARR || sym->type == symbol_type::PROC_ARRAY_ARGUMENT) {  // array as index
    throw std::runtime_error("Error at line " + std::to_string(line_number) + ": array index cannot be an array.");
  }
  if(sym->type == symbol_type::VAR && !sym->initialized) {  // index variable not initialized
    throw std::runtime_error("Error at line " + std::to_string(line_number) + ": variable " +
        sym->symbol_name + " used as index is not initialized.");
  }
  if(sym->type == symbol_type::PROC_ARGUMENT && !sym->initialized) {
    markProcedureArgumentNeedsInitialization(variable_name);
  }
  VariableIndexedArray * arr = new VariableIndexedArray;
  arr->type = variable_type::VARIABLE_INDEXED_ARR;
  arr->var_name = variable_name;
  arr->index_var_name = index_variable_name;
  return arr;
}

VariableContainer *Compiler::checkVariableInitialization(VariableContainer *var, int line_number) {
  if(var->type == variable_type::R_VAL) {  // rvalue is not an identifier
    throw std::runtime_error("Error at line " + std::to_string(line_number) + ": const value is not a variable.");
  }
  std::shared_ptr<Symbol> sym = current_symbol_table_->findSymbol(var->getVariableName());
  if(sym->type == symbol_type::VAR && !sym->initialized) {
    throw std::runtime_error("Error at line " + std::to_string(line_number) + ": variable "
                                 + var->getVariableName() + " is uninitialized");
  } else if(sym->type == symbol_type::PROC_ARGUMENT) {
    markProcedureArgumentNeedsInitialization(var->getVariableName());
  }
  return var;
}

void Compiler::outputCode(std::vector<std::shared_ptr<GraphNode>> start_nodes) {
  std::fstream f;
  f.open(output_file_name_, std::ios::out);
  if(start_nodes.size() > 1) {
    f << "JUMP " << start_nodes.at(start_nodes.size() - 1)->start_line_ << std::endl;
  }
  for(auto start_node : start_nodes) {
    outputGraphRecursively(start_node, f);
  }
  f << "HALT" << std::endl;
  f.close();
}

void Compiler::outputGraphRecursively(std::shared_ptr<GraphNode> curr_node, std::fstream &f_out) {
  for(auto line : curr_node->code_list_) {
    f_out << line << std::endl;
  }
  if(curr_node->left_node)
    outputGraphRecursively(curr_node->left_node, f_out);
  if(curr_node->right_node)
    outputGraphRecursively(curr_node->right_node, f_out);
  if(curr_node->jump_line_target) {
    f_out << "JUMP " << curr_node->jump_line_target->start_line_ << std::endl;
  } else if(curr_node->jump_condition_target) {
    f_out << "JUMP " << curr_node->jump_condition_target->condition_start_line_ << std::endl;
  }
}

Symbol Compiler::createSymbol(std::string symbol_name, enum symbol_type type) {
  Symbol new_symbol;
  new_symbol.symbol_name = symbol_name;
  new_symbol.type = type;
  new_symbol.initialized = false;
  return new_symbol;
}

// TODO(Jakub Drzewiecki): Set symbols start indexes at next powers of 2 (if there are less than 64)
void Compiler::setSymbolBounds(Symbol & symbol, long long mem_len, int line_number) {
  if(mem_len <= 0) {  // illegal array size
    throw std::runtime_error("Error at line " + std::to_string(line_number)
    + ": array size has to be a positive number.");
  }
  symbol.mem_start = curr_memory_offset_;
  symbol.length += mem_len;
  if(curr_memory_offset_ + mem_len < curr_memory_offset_) {  // memory offset overflow
    throw std::runtime_error("Error at line " + std::to_string(line_number)
    + ": memory capacity reached.");
  }
  curr_memory_offset_ += mem_len;
  if(curr_memory_offset_ > k_max_mem_offset) {  // used too much memory
    throw std::runtime_error("Error at line " + std::to_string(line_number)
    + ": memory capacity reached.");
  }
}

void Compiler::markProcedureArgumentNeedsInitialization(std::string arg_name) {
  for(auto & arg : current_procedure_arguments_) {
    if(arg.name == arg_name) {
      arg.needs_initialization_before_call = true;
      return;
    }
  }
}

bool Compiler::isProcedureArgumentMarked(std::string arg_name) {
  for(auto & arg : current_procedure_arguments_) {
    if(arg.name == arg_name) {
      return arg.needs_initialization_before_call;
    }
  }
  return false;
}