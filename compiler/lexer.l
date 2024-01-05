/*
 *
*/
%option noyywrap
%option yylineno
%option nounput
%{
#include "parser.hpp"

int yylex();
%}

NUMBER ([1-9][0-9]*|0)
SPECIAL_SIGNS [\;,\(\)\[\]]
ASSIGNMENT ":="
ARRAY_PARAMETER "T"
PIDENTIFIER [_a-z]+
COMMENT #.*$

%%

"PROCEDURE" { return PROCEDURE; }
"IS" { return IS; }
"IN" { return IN; }
"END" { return END; }

"PROGRAM" { return PROGRAM; }

"IF" { return IF; }
"THEN" { return THEN; }
"ELSE" { return ELSE; }
"ENDIF" { return ENDIF; }

"WHILE" { return WHILE; }
"DO" { return DO; }
"ENDWHILE" { return ENDWHILE; }

"REPEAT" { return REPEAT; }
"UNTIL" { return UNTIL; }

"READ" { return READ; }
"WRITE" { return WRITE; }

"+" { return PLUS; }
"-" { return MINUS; }
"*" { return TIMES; }
"/" { return DIV; }
"%" { return MOD; }

"=" { return EQ; }
"!=" { return NEQ; }
">" { return GT; }
"<" { return LT; }
">=" { return GE; }
"<=" { return LE; }

{ASSIGNMENT} { return ASSIGNMENT; }
{SPECIAL_SIGNS} { return yytext[0]; }
{ARRAY_PARAMETER} { return yytext[0]; }
{PIDENTIFIER} { yylval.pidentifier = new std::string(yytext); return pidentifier; }
{COMMENT} ;

{NUMBER} { yylval.num = std::stoll(yytext); return num; }


[ \t\r]+ ;
[\n]+  ;
.               return ERROR;
%%
