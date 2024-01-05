/*
 * Parser for given language.
 * It parses language to pivot language and outputs final assembler code.
*/
%code requires {
#include <iostream>
#include <vector>
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
    class Command* comm;
    std::vector<Command*>* comm_list;
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
%type <comm> command
%type <comm_list> commands

%%

/* Problems:
 * store code in data structures
 * transform data structures into flow graph
 * translate flow graph into result code
 * add optimizations in translation phase
 */

program_all  : procedures main {/* generate flow graph, optimize and output code */}
             ;

procedures   : procedures PROCEDURE proc_head IS declarations IN commands END { compiler->declareProcedure(*$7); }
             | procedures PROCEDURE proc_head IS IN commands END { compiler->declareProcedure(*$6); }
             | %empty
             ;

main         : PROGRAM IS declarations IN commands END { compiler->declareMain(*$5); }
             | PROGRAM IS IN commands END { compiler->declareMain(*$4); }
             ;

commands     : commands command { $1->push_back($2); }
             | command { $$ = new std::vector<Command*>(); $$->push_back($1); }
             ;

command      : identifier ASSIGNMENT expression';' { $$ = compiler->createAssignmentCommand($1, $3, yylineno);}
             | IF condition THEN commands ELSE commands ENDIF { $$ = compiler->createIfThenElseBlock($2, *$4, *$6, yylineno); }
             | IF condition THEN commands ENDIF { $$ = compiler->createIfThenElseBlock($2, *$4, yylineno); }
             | WHILE condition DO commands ENDWHILE { compiler->createWhileBlock($2, *$4, yylineno); }
             | REPEAT commands UNTIL condition';' { compiler->createRepeatUntilBlock($4, *$2, yylineno); }
             | proc_call';' { compiler->createProcedureCallCommand(yylineno); }
             | READ identifier';' { compiler->createReadCommand($2, yylineno); }
             | WRITE value';' { compiler->createWriteCommand($2, yylineno); }
             ;

proc_head    : pidentifier '('args_decl')' { compiler->declareProcedureHead(*$1, yylineno); }
             ;

proc_call    : pidentifier '('args')' { compiler->createProcedureCall(*$1, yylineno); }
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

args         : args',' pidentifier { compiler->addProcedureCallArgument(*$3, yylineno); }
             | pidentifier { compiler->addProcedureCallArgument(*$1, yylineno); }
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

