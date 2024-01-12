#ifndef CUSTOMCOMPILER_COMPILER_DATA_H_
#define CUSTOMCOMPILER_COMPILER_DATA_H_

#include <iostream>
#include <memory>
#include "symbol.h"
#include "symbol_table.h"

enum class variable_type {
  VAR,
  R_VAL,
  ARR,
  VARIABLE_INDEXED_ARR
};

typedef struct variable_container {
  variable_type type;
  virtual std::string getVariableName() {
    return "";
  };
  virtual std::string getIndexVariableName() {
    return "";
  };
  virtual long long int getValue() {
    return -1;
  }
} VariableContainer;

typedef struct variable : public VariableContainer {
  std::string var_name;
  std::string getVariableName() override {
    return var_name;
  };
} Variable;

typedef struct r_value : public VariableContainer {
  long long int value;
  long long getValue() override {
    return value;
  }
} RValue;

typedef struct array : public VariableContainer {
  std::string var_name;
  long long int index;
  std::string getVariableName() override {
    return var_name;
  };
  long long getValue() override {
    return index;
  }
} Array;

typedef struct variable_indexed_array : public VariableContainer {
  std::string var_name;
  std::string index_var_name;

  std::string getVariableName() override {
    return var_name;
  };
  std::string getIndexVariableName() override {
    return index_var_name;
  }
} VariableIndexedArray;

class Register {
 public:
  bool currently_used_ = false;
  bool variable_saved_ = true;
  std::string register_name_;
  std::shared_ptr<VariableContainer> curr_variable = nullptr;
};

enum class expression_type {
  DEFAULT,
  PLUS,
  MINUS,
  MULTIPLY,
  DIVIDE,
  MODULO
};

// TODO(Jakub Drzewiecki): change expressions to two classes
/**
 * x := var_  (x - left side variable)
 *
 * get var_ into reg_?
 *
 * x is in reg_?
 */
class DefaultExpression {
 public:
  VariableContainer* var_;
  virtual std::vector<VariableContainer> neededVariablesInRegisters() {
    return {*var_};
  }

  // accumulator should be the last register
  virtual std::vector<std::string> calculateExpression(std::vector<std::shared_ptr<Register>> regs,
                                                       long long int expression_first_line_number) {
    return {};
  }

  virtual int neededEmptyRegs() {
    return 0;
  }
 private:
  expression_type type = expression_type::DEFAULT;
};

/**
 * x := var_ + right_var_
 *
 * get var_ into reg_b
 * get right_var_ into reg_a
 * ADD b
 *
 * x is in reg a
 */
class PlusExpression : public DefaultExpression {
 public:
  VariableContainer* right_var_;
  std::vector<VariableContainer> neededVariablesInRegisters() override {
    return {*var_, *right_var_};
  }

  std::vector<std::string> calculateExpression(std::vector<std::shared_ptr<Register>> regs,
                                               long long int expression_first_line_number) override {
    std::shared_ptr<Register> var_register = regs.at(0);
    return {"ADD " + var_register->register_name_};
  }

  int neededEmptyRegs() override {
    return 0;
  }
 private:
  expression_type type = expression_type::PLUS;
};

/**
 * x := var_ + right_var_
 *
 * get right_var_ into reg_b
 * get var_ into reg_a
 * SUB b
 *
 * x is in reg a
 */
class MinusExpression : public DefaultExpression {
 public:
  VariableContainer* right_var_;
  std::vector<VariableContainer> neededVariablesInRegisters() override {
    return {*right_var_, *var_};
  }

  std::vector<std::string> calculateExpression(std::vector<std::shared_ptr<Register>> regs,
                                               long long int expression_first_line_number) override {
    std::shared_ptr<Register> right_var_register = regs.at(0);
    return {"SUB " + right_var_register->register_name_};
  }

  int neededEmptyRegs() override {
    return 0;
  }
 private:
  expression_type type = expression_type::MINUS;
};

/**
 * x := var_ * right_var_
 *
 * multiplication requires 5 registers (including accumulator)
 * var in reg_b
 * right_var in reg_a
 * use reg_c to store right_var
 * use reg_d to store lower value from var and right_var, used as iterator for loop end
 * use reg_e to store result
 *
 * PUT reg_c
 * INC reg_a
 * SUB reg_b
 * JPOS right_var_greater_than_var
 * GET reg_b  # switch reg_b and reg_c
 * PUT reg_e
 * GET reg_c
 * PUT reg_b
 * GET reg_e
 * PUT reg_c
 * GET reg_b  # check if value of lesser variable is 0 at start, right_var_greater_than_var
 * JZERO end_of_multiplication  (result is 0)
 * PUT reg_d
 * RST reg_e
 * GET reg_d  # multiplication_start
 * JZERO parse_result
 * GET reg_d
 * SHR reg_b
 * SHL reg_b
 * SUB reg_b
 * SHR reg_d
 * SHR reg_b
 * JZERO shift_left_added_value (last bit of reg_b is zero)
 * GET reg_e
 * ADD reg_c
 * PUT reg_e
 * SHL reg_c  # shift_left_added_value
 * JUMP multiplication_start
 * GET reg_e  # parse_result
 * end_of_multiplication
 *
 * x is in reg a
 */
class MultiplyExpression : public DefaultExpression {
 public:
  VariableContainer* right_var_;
  std::vector<VariableContainer> neededVariablesInRegisters() override {
    return {*var_, *right_var_};
  }
  std::vector<std::string> calculateExpression(std::vector<std::shared_ptr<Register>> regs,
                                               long long int expression_first_line_number) override {
    std::shared_ptr<Register> var_reg = regs.at(0);
    std::shared_ptr<Register> acc_reg = regs.at(1);
    std::shared_ptr<Register> right_var_reg = regs.at(2);
    std::shared_ptr<Register> iterator_reg = regs.at(3);
    std::shared_ptr<Register> result_reg = regs.at(0);
    std::vector<std::string> result_code {
      "PUT " + right_var_reg->register_name_,
      "INC " + acc_reg->register_name_,
      "SUB " + var_reg->register_name_,
      "JPOS " + std::to_string(expression_first_line_number + 10),
      "GET " + var_reg->register_name_,
      "PUT " + result_reg->register_name_,
      "GET " + right_var_reg->register_name_,
      "PUT " + var_reg->register_name_,
      "GET " + result_reg->register_name_,
      "PUT " + right_var_reg->register_name_,
      "GET " + var_reg->register_name_,
      "JZERO " + std::to_string(expression_first_line_number + 29),
      "PUT " + iterator_reg->register_name_,
      "RST " + result_reg->register_name_,
      "GET " + iterator_reg->register_name_,
      "JZERO " + std::to_string(expression_first_line_number + 28),
      "GET " + iterator_reg->register_name_,
      "SHR " + var_reg->register_name_,
      "SHL " + var_reg->register_name_,
      "SUB " + var_reg->register_name_,
      "SHR " + iterator_reg->register_name_,
      "SHR " + var_reg->register_name_,
      "JZERO " + std::to_string(expression_first_line_number + 26),
      "GET " + result_reg->register_name_,
      "ADD " + right_var_reg->register_name_,
      "PUT " + result_reg->register_name_,
      "SHL " + right_var_reg->register_name_,
      "JUMP " + std::to_string(expression_first_line_number + 14),
      "GET " + result_reg->register_name_
    };
    return result_code;
  }

  int neededEmptyRegs() override {
    return 3;
  }
 private:
  expression_type type = expression_type::MULTIPLY;
};

/**
 * x := var_ / right_var_
 *
 * x is in reg a
 */
class DivideExpression : public DefaultExpression {
 public:
  VariableContainer* right_var_;
  std::vector<VariableContainer> neededVariablesInRegisters() override {
    return {*var_, *right_var_};
  }
 private:
  expression_type type = expression_type::DIVIDE;
};

/**
 * x := var_ % right_var_
 *
 * x is in reg a
 */
class ModuloExpression : public DefaultExpression {
 public:
  VariableContainer* right_var_;
  std::vector<VariableContainer> neededVariablesInRegisters() override {
    return {*var_, *right_var_};
  }
 private:
  expression_type type = expression_type::MODULO;
};

/**
 * a ? b
 * ---------
 * EQ: a - b == 0 && b - a == 0
 *
 * LOAD b_add
 * PUT b
 * LOAD a_add
 * PUT c
 * SUB b
 * JPOS next_block  (a - b > 0 -> a != b; a - b == 0 -> b - a ? 0)
 * GET b
 * SUB c
 * JPOS next_block (b - a > 0 -> a != b; b - a == 0 -> a == b)
 * equal block
 * next_block
 * -----------------
 * NEQ: a - b != 0 || b - a != 0
 *
 * LOAD b_add
 * PUT b
 * LOAD a_add
 * PUT c
 * SUB b
 * JPOS nequal_block  (a - b > 0 -> a != b; a - b == 0 -> b - a ? 0)
 * GET b
 * SUB c
 * JZERO next_block  (b - a == 0 -> a == b ; b - a > 0 -> a != b)
 * nequal_block
 * next_block
 * ------------------
 * GT:  a > b -> !(a <= b) -> !(a < b + 1) -> !(b + 1 - a > 0)
 *
 * LOAD a_add
 * PUT b
 * LOAD b_add
 * INC a
 * SUB b  (b + 1 - a)
 * JPOS next_block
 * gt_block
 * next_block
 * -------------------------
 * GE: a >= b -> a + 1 > b -> a + 1 - b > 0
 *
 * LOAD b_add
 * PUT b
 * LOAD a_add
 * INC a
 * SUB b  (a + 1 - b)
 * JZERO next_block  (a + 1 - b == 0 -> a + 1 <= b)
 * ge_block
 * next_block
 */
enum class condition_type {
  EQ,
  NEQ,
  GT,
  GE
};

// TODO(Jakub Drzewiecki): Optimize conditions if they are always false/true
class Condition {
 public:
  condition_type type_;
  VariableContainer* left_var_;
  VariableContainer* right_var_;
  std::vector<VariableContainer> neededVariablesInRegisters() {
    if(type_ == condition_type::GT) {
      return {*left_var_, *right_var_};
    }
    return {*right_var_, *left_var_};
  }

  long long int getConditionCodeSize() {
    switch(type_) {
      case condition_type::EQ:
        return 6;
      case condition_type::NEQ:
        return 6;
      case condition_type::GT:
        return 3;
      case condition_type::GE:
        return 3;
    }
    return 0;
  }

  std::vector<std::string> generateCondition(std::vector<std::shared_ptr<Register>> registers,
                                             long long int target_block_start_line_number,
                                             long long int next_block_start_line_number) {
    std::vector<std::string> result_code;
    std::shared_ptr<Register> first = registers.at(0);
    std::shared_ptr<Register> accumulator = registers.at(1);
    std::shared_ptr<Register> free_reg = registers.at(2);
    switch(type_) {
      case condition_type::EQ:
        result_code.push_back("PUT " + free_reg->register_name_);
        free_reg->curr_variable = accumulator->curr_variable;
        free_reg->variable_saved_ = accumulator->variable_saved_;
        result_code.push_back("SUB " + first->register_name_);
        result_code.push_back("JPOS " + std::to_string(next_block_start_line_number));
        result_code.push_back("GET " + first->register_name_);
        result_code.push_back("SUB " + free_reg->register_name_);
        result_code.push_back("JPOS " + std::to_string(next_block_start_line_number));
        accumulator->variable_saved_ = true;
        accumulator->curr_variable = nullptr;
      case condition_type::NEQ:
        result_code.push_back("PUT " + free_reg->register_name_);
        free_reg->curr_variable = accumulator->curr_variable;
        free_reg->variable_saved_ = accumulator->variable_saved_;
        result_code.push_back("SUB " + first->register_name_);
        result_code.push_back("JPOS " + std::to_string(target_block_start_line_number));
        result_code.push_back("GET " + first->register_name_);
        result_code.push_back("SUB " + free_reg->register_name_);
        result_code.push_back("JZERO " + std::to_string(next_block_start_line_number));
        accumulator->variable_saved_ = true;
        accumulator->curr_variable = nullptr;
      case condition_type::GT:
        result_code.push_back("INC " + accumulator->register_name_);
        result_code.push_back("SUB " + first->register_name_);
        result_code.push_back("JPOS " + std::to_string(next_block_start_line_number));
      case condition_type::GE:
        result_code.push_back("INC " + accumulator->register_name_);
        result_code.push_back("SUB " + first->register_name_);
        result_code.push_back("JZERO " + std::to_string(next_block_start_line_number));
    }
    return result_code;
  }
};

typedef struct procedure_argument {
  std::string name;
  enum symbol_type type;
  bool needs_initialization_before_call;
} ProcedureArgument;

typedef struct procedure_head {
  std::string name;
  std::vector<ProcedureArgument> arguments;
} ProcedureHead;

typedef struct procedure_call_argument {
  std::string name;
  enum symbol_type type;
  bool needs_initialization_before_call;
  std::shared_ptr<Symbol> target_variable_symbol;
} ProcedureCallArgument;

typedef struct procedure_call {
  std::string name;
  std::vector<ProcedureCallArgument> args;
} ProcedureCall;

enum class command_type {
  ASSIGNMENT,
  IF_ELSE,
  WHILE,
  REPEAT,
  PROC_CALL,
  READ,
  WRITE
};

class Command {
 public:
  enum command_type type;
};

/**
 * x := a ? b
 * ? - expression
 * y_add - address of y variable
 *
 * LOAD a_add
 * ? b (store expression result in R_a)
 * STORE x_add
 */
class AssignmentCommand : public Command {
 public:
  VariableContainer left_var_;
  DefaultExpression expression_;
};

/**
 * if else then:
 * condition target block - then_commands
 * condition next block - else_commands
 *
 * cond-block
 * then_commands
 * JUMP next_block
 * else_commands
 * next_block
 *
 * ----------------
 * if then:
 * condition target block - then commands
 * condition next block - next block
 *
 * cond-block
 * then_commands
 * next_block
 */
class IfElseCommand : public Command {
 public:
  Condition cond_;
  std::vector<Command*> then_commands_;
  std::vector<Command*> else_commands_;
};

/**
 * condition target block - commands
 * condition next block - next_block
 *
 * cond-block
 * commands
 * JUMP cond-block
 * next_block
 */
class WhileCommand : public Command {
 public:
  Condition cond_;
  std::vector<Command*> commands_;
};

/**
 * condition target block - loop_end
 * condition next block - commands
 *
 * cond-block
 * loop_end
 * JUMP next_block
 * commands
 * JUMP cond_block
 * next_block
 */
class RepeatUntilCommand : public Command {
 public:
  Condition cond_;
  std::vector<Command*> commands_;
};

/**
 * firstly store all current registers that hold values in memory
 *
 * STRK h  (store current line number)
 * JUMP proc_start
 * procedure
 * INC h
 * INC h
 * GET h
 * JUMPR h
 *
 */
class ProcedureCallCommand : public Command {
 public:
  ProcedureCall proc_call_;
};

/**
 * READ a:
 *
 * READ
 * STORE a_add
 */
class ReadCommand : public Command {
 public:
  VariableContainer var_;
};

/**
 * WRITE a:
 *
 * LOAD a_add
 * WRITE
 */
class WriteCommand : public Command {
 public:
  VariableContainer written_value_;
};

typedef struct procedure {
  ProcedureHead head;
  std::shared_ptr<SymbolTable> symbol_table;
  std::vector<Command*> commands;
} Procedure;

#endif  // CUSTOMCOMPILER_COMPILER_DATA_H_
