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
  proc.proc_type = procedure_type::PROC;
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