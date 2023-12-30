/*
 * Parser for given language.
 * It parses language to pivot language and outputs final assembler code.
*/
%code requires {
#include <iostream>
}

%{
#include <iostream>

extern int yylex();
extern int yylineno;
extern FILE *yyin;
void yyerror( std::string s );

%}

%union {
    std::string *pidentifier;
    long long int num;
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

%%

program_all  : procedures main
             ;

procedures   : procedures PROCEDURE proc_head IS declarations IN commands END
             | procedures PROCEDURE proc_head IS IN commands END
             | %empty
             ;

main         : PROGRAM IS declarations IN commands END
             | PROGRAM IS IN commands END
             ;

commands     : commands command
             | command
             ;

command      : identifier ASSIGNMENT expression';'
             | IF condition THEN commands ELSE commands ENDIF
             | IF condition THEN commands ENDIF
             | WHILE condition DO commands ENDWHILE
             | REPEAT commands UNTIL condition';'
             | proc_call';'
             | READ identifier';'
             | WRITE value';'
             ;

proc_head    : pidentifier '('args_decl')'
             ;

proc_call    : pidentifier '('args')'
             ;

declarations : declarations',' pidentifier
             | declarations',' pidentifier'['num']'
             | pidentifier
             | pidentifier'['num']'
             ;

args_decl    : args_decl',' pidentifier
             | args_decl',' 'T' pidentifier
             | pidentifier
             | 'T' pidentifier
             ;

args         : args',' pidentifier
             | pidentifier
             ;

expression   : value
             | value PLUS value
             | value MINUS value
             | value TIMES value
             | value DIV value
             | value MOD value
             ;

condition    : value EQ value
             | value NEQ value
             | value GT value
             | value LT value
             | value GE value
             | value LE value
             ;

value        : num
             | identifier
             ;

identifier   : pidentifier
             | pidentifier'['num']'
             | pidentifier'['pidentifier']'
             ;

%%

void yyerror( std::string s )
{
  std::cout << s << std::endl;
}

int main(int argv, char* argc[]) {

    yyin = fopen(argc[1], "r");
    if (yyin == NULL){
        std::cout << "Plik nie istnieje" << std::endl;
        return 1;
    }

	yyparse();
    return 0;
}

