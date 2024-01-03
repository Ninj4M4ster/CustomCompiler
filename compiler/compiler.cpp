//
// Created by drzewo on 31.12.23.
//

#include "compiler.h"

Compiler::Compiler() {
  current_symbol_table_ = std::make_shared<SymbolTable>();
}

void Compiler::setOutputFileName(std::string f_name) {
  output_file_name_ = f_name;
}

void Compiler::declareProcedure() {
  Procedure proc;
  proc.head = curr_procedure_head_;
  curr_procedure_head_ = ProcedureHead{};
  proc.commands = current_commands_;
  current_commands_.clear();
  proc.symbol_table = current_symbol_table_;
  current_symbol_table_ = std::make_shared<SymbolTable>();
  procedures_.push_back(proc);
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
  current_symbol_table_->addSymbol(new_symbol, line_number);
  current_procedure_arguments_.push_back({variable_name, symbol_type::VAR});
}

void Compiler::declareProcedureArrayArgument(std::string variable_name, int line_number) {
  Symbol new_symbol = createSymbol(variable_name, symbol_type::PROC_ARRAY_ARGUMENT);
  current_symbol_table_->addSymbol(new_symbol, line_number);
  current_procedure_arguments_.push_back({variable_name, symbol_type::ARR});
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
  Array * arr = new Array;
  arr->type = variable_type::ARR;
  arr->var_name = variable_name;
  arr->index = index;
  return arr;
}

VariableContainer *Compiler::getVariable(std::string variable_name, std::string index_variable_name, int line_number) {
  std::shared_ptr<Symbol> sym = current_symbol_table_->findSymbol(variable_name);
  if(sym == nullptr) {  // no variable with given name found
    throw std::runtime_error("Error at line " + std::to_string(line_number) + ": unknown variable name.");
  }
  if(sym->type == symbol_type::VAR ||
      sym->type == symbol_type::PROC_ARGUMENT) {  // normal variable accessed as array
    throw std::runtime_error("Error at line " + std::to_string(line_number) + ": variable "
                                 + variable_name + " of decimal type accessed array-like.");
  }
  // check var_index symbol
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
  if(!sym->initialized) {
    throw std::runtime_error("Error at line " + std::to_string(line_number) + ": variable "
                                 + variable_name + " is uninitialized");
  }
  return var;
}

Symbol Compiler::createSymbol(std::string symbol_name, enum symbol_type type) {
  Symbol new_symbol;
  new_symbol.symbol_name = symbol_name;
  new_symbol.type = type;
  new_symbol.initialized = false;
  return new_symbol;
}

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