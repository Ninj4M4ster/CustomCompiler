/*
 * Parser for given language.
 * It parses language to pivot language and outputs final assembler code.
*/
%code requires {
#include <iostream>
}

%{
#include "compiler.h"

extern int yylex();
extern int yylineno;
extern FILE *yyin;
void yyerror( std::string s );
std::shared_ptr<Compiler> compiler;

%}

%union {
    std::string *pidentifier;
    long long int num;
    struct variable_container* var_container;
    class DefaultExpression* expr;
    class Condition* cond;
}

%token PROCEDURE IS IN END
%token PROGRAM
%token IF THEN ELSE ENDIF
%token WHILE DO ENDWHILE
%token REPEAT UNTIL
%token READ WRITE
%token<pidentifier> pidentifier
%token<num> num

%token ASSIGNMENT

%left PLUS MINUS
%left TIMES DIV MOD

%token EQ NEQ GT GE LT LE

%token ERROR

%type <var_container> value identifier
%type <expr> expression
%type <cond> condition

%%

/* Problems:
 * store code in data structures
 * transform data structures into flow graph
 * translate flow graph into result code
 * add optimizations in translation phase
 */

program_all  : procedures main {/* generate flow graph, optimize and output code */}
             ;

procedures   : procedures PROCEDURE proc_head IS declarations IN commands END { compiler->declareProcedure(); }
             | procedures PROCEDURE proc_head IS IN commands END { compiler->declareProcedure(); }
             | %empty
             ;

main         : PROGRAM IS declarations IN commands END {/* generate main function and pass it to $$ */}
             | PROGRAM IS IN commands END {/* generate main function and pass it to $$ */}
             ;

commands     : commands command {/* add new command from $2 to $1 */}
             | command {/* create commands list and pass it to $$ */}
             ;

command      : identifier ASSIGNMENT expression';' {/* pass assignment command to $$, initialize identifier */}
             | IF condition THEN commands ELSE commands ENDIF {/* pass if then else nodes to $$ */}
             | IF condition THEN commands ENDIF {/* pass if then nodes to $$ */}
             | WHILE condition DO commands ENDWHILE {/* pass while do nodes to $$ */}
             | REPEAT commands UNTIL condition';' {/* pass repeat until node to $$ */}
             | proc_call';' {/* pass proc call node to $$ */}
             | READ identifier';' {/* pass read command to $$, initialize identifier */}
             | WRITE value';' {/* pass write command to $$ */}
             ;

proc_head    : pidentifier '('args_decl')' { compiler->declareProcedureHead(*$1, yylineno); }
             ;

proc_call    : pidentifier '('args')' {/* create function object with given arguments to $$, check if array indexes are in bounds */}
             ;

declarations : declarations',' pidentifier { compiler->declareVariable(*$3, yylineno); }
             | declarations',' pidentifier'['num']' { compiler->declareVariable(*$3, $5, yylineno); }
             | pidentifier { compiler->declareVariable(*$1, yylineno); }
             | pidentifier'['num']' { compiler->declareVariable(*$1, $3, yylineno); }
             ;

args_decl    : args_decl',' pidentifier { compiler->declareProcedureArgument(*$3, yylineno); }
             | args_decl',' 'T' pidentifier { compiler->declareProcedureArrayArgument(*$4, yylineno); }
             | pidentifier { compiler->declareProcedureArgument(*$1, yylineno); }
             | 'T' pidentifier { compiler->declareProcedureArrayArgument(*$2, yylineno); }
             ;

args         : args',' pidentifier {/* pass argument from $3 to args */}
             | pidentifier {/* pass argument to $$ */}
             ;

expression   : value { $$ = compiler->createDefaultExpression($1, yylineno); }
             | value PLUS value { $$ = compiler->createPlusExpression($1, $3, yylineno); }
             | value MINUS value { $$ = compiler->createMinusExpression($1, $3, yylineno); }
             | value TIMES value { $$ = compiler->createMultiplyExpression($1, $3, yylineno); }
             | value DIV value { $$ = compiler->createDivideExpression($1, $3, yylineno); }
             | value MOD value { $$ = compiler->createModuloExpression($1, $3, yylineno); }
             ;

condition    : value EQ value { $$ = compiler->createEqualCondition($1, $3, yylineno); }
             | value NEQ value { $$ = compiler->createNotEqualCondition($1, $3, yylineno); }
             | value GT value { $$ = compiler->createGreaterCondition($1, $3, yylineno); }
             | value LT value { $$ = compiler->createGreaterCondition($3, $1, yylineno); }
             | value GE value { $$ = compiler->createGreaterEqualCondition($1, $3, yylineno); }
             | value LE value { $$ = compiler->createGreaterEqualCondition($3, $1, yylineno); }
             ;

value        : num { $$ = compiler->getVariable($1,  yylineno); }
             | identifier { $$ = compiler->checkVariableInitialization($1, yylineno); }
             ;

identifier   : pidentifier { $$ = compiler->getVariable(*$1, yylineno); }
             | pidentifier'['num']' { $$ = compiler->getVariable(*$1, $3, yylineno); }
             | pidentifier'['pidentifier']' { $$ = compiler->getVariable(*$1, *$3, yylineno); }
             ;

%%

void yyerror( std::string s )
{
  std::cout << s << std::endl;
}

int main(int argc, char* argv[]) {
    compiler = std::make_shared<Compiler>();

    if(argc != 3) {
        std::cout << "Bad number of program arguments\n Usage: compiler <input_file_name> <output_file_name>\n";
        return 1;
    }

    yyin = fopen(argv[1], "r");
    if (yyin == NULL){
        std::cout << "Input file does not exist" << std::endl;
        return 1;
    }

    std::string output_file_name = std::string(argv[2]);
    compiler->setOutputFileName(output_file_name);

	yyparse();
    return 0;
}

