#include "symbol_table.h"

void SymbolTable::addSymbol(Symbol new_symbol, int line_number) {
  for(auto & s : symbol_table_list_) {
    if(s->symbol_name == new_symbol.symbol_name) {
      throw std::runtime_error("Error at line " + std::to_string(line_number) +
      ": duplicate variable definition - " + s->symbol_name);
    }
  }
  symbol_table_list_.push_back(std::make_shared<Symbol>(new_symbol));
}

std::shared_ptr<Symbol> SymbolTable::findSymbol(std::string symbol_name) {
  for(auto & s : symbol_table_list_) {
    if(s->symbol_name == symbol_name) {
      return s;
    }
  }
  return nullptr;
}