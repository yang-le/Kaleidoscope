%{
#include <stdio.h>
#include "ast.h"

int yylex(void);
void yyerror(const char* msg);

ExprListAST* root;
%}

%union {
    std::string* id;
    double num;
    ExprAST* expr;
    ExprListAST* expr_list;
    VariableListAST* id_list;
    PrototypeAST* proto;
}

%token DEF EXTERN
%token <id> ID
%token <num> NUM

%type <expr> expr stmt
%type <expr_list> expr_list stmt_list
%type <id_list> id_list
%type <proto> proto

%left '+' '-'
%left '*' '/'

%%
prog:
    stmt_list               { root = $1; }
    ;

stmt_list:
    stmt                    { $$ = new ExprListAST($1); }
    | stmt_list stmt        { $$ = $1; $1->push_back($2); }
    |                       { $$ = new ExprListAST(); }
    ;

stmt:
    DEF proto expr          { $$ = new FunctionAST($2, $3); }
    | EXTERN proto          { $$ = $2; }
    | expr                  { $$ = new FunctionAST(new PrototypeAST("__anon_expr", new VariableListAST()), $1);}
    ;

expr:
    '(' expr ')'            { $$ = $2; }
    | expr '+' expr         { $$ = new BinaryExprAST('+', $1, $3); }
    | expr '-' expr         { $$ = new BinaryExprAST('-', $1, $3); }
    | expr '*' expr         { $$ = new BinaryExprAST('*', $1, $3); }
    | expr '/' expr         { $$ = new BinaryExprAST('/', $1, $3); }
    | NUM                   { $$ = new NumberExprAST($1); }
    | ID                    { $$ = new VariableExprAST(*$1); }
    | ID '(' expr_list ')'  { $$ = new CallExprAST(*$1, $3); }
    ;

expr_list:
    expr                    { $$ = new ExprListAST($1); }
    | expr_list ',' expr    { $$ = $1; $1->push_back($3); }
    |                       { $$ = new ExprListAST(); }
    ;

id_list:
    ID                      { $$ = new VariableListAST(*$1); }
    | id_list ',' ID        { $$ = $1; $1->push_back(*$3); }
    |                       { $$ = new VariableListAST(); }
    ;

proto:
    ID '(' id_list ')'      { $$ = new PrototypeAST(*$1, $3); }
    ;
%%

void yyerror(const char* msg)
{
    fprintf(stderr, "%s\n", msg);
}

int main(int argc, char** argv)
{
#if YYDEBUG
    yydebug = 1;
#endif

    yyparse();

    if (root)
        root->codegen();

    codedump();

    return 0;
}
