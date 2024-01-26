#ifndef CUSTOMCOMPILER_COMPILER_SYMBOL_H_
#define CUSTOMCOMPILER_COMPILER_SYMBOL_H_

#include <iostream>
#include <vector>

enum symbol_type {
  ARR,
  VAR,
  PROC_ARGUMENT,
  PROC_ARRAY_ARGUMENT
};

typedef struct symbol {
  std::string symbol_name;
  enum symbol_type type;
  bool initialized;
  size_t mem_start;
  size_t length;
  bool proc_jump_back_mem = false;
} Symbol;

#endif  // CUSTOMCOMPILER_COMPILER_SYMBOL_H_
