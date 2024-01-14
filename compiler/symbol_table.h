#ifndef CUSTOMCOMPILER_COMPILER_SYMBOL_TABLE_H_
#define CUSTOMCOMPILER_COMPILER_SYMBOL_TABLE_H_

#include <memory>
#include <vector>
#include "symbol.h"

// TODO(Jakub Drzewiecki): Add symbol for line address for jumping out of procedures
class SymbolTable {
 public:
  void addSymbol(Symbol new_symbol, int line_number);
  void addProcJumpBackMemoryAddress(Symbol new_symbol);
  std::shared_ptr<Symbol> findSymbol(std::string symbol_name);
  std::shared_ptr<Symbol> getProcedureJumpBackMemoryAddressSymbol();
  void output_symbols();

 private:
  std::vector<std::shared_ptr<Symbol>> symbol_table_list_;
};

#endif  // CUSTOMCOMPILER_COMPILER_SYMBOL_TABLE_H_
