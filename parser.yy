%skeleton "lalr1.cc" /* -*- C++ -*- */
%require "3.5.1"
%defines

%define api.token.constructor
%define api.value.type variant
%define parse.assert

%code requires {
  #include <string>
  #include "ast.hh"
  class driver;
}

// The parsing context.
%param { driver& drv }

%locations

%define parse.trace
%define parse.error verbose

%code {
# include "driver.hh"
}

%define api.token.prefix {TOK_}
%token
  END 0   "end of file"
  ASSIGN  "="
  MINUS   "-"
  PLUS    "+"
  STAR    "*"
  SLASH   "/"
  LPAREN  "("
  RPAREN  ")"
  COMMA   ","
  DEF     "def"
  EXTERN  "extern"

%token <std::string> ID "identifier"
%token <double> NUM "number"

%nterm <std::unique_ptr<ExprAST>> expr stmt
%nterm <std::unique_ptr<PrototypeAST>> proto
%nterm <std::vector<std::unique_ptr<ExprAST>>> expr_list stmt_list
%nterm <std::vector<std::string>> id_list

/* %printer { yyo << $$; } <*> */

%%
%start prog;
prog:
    stmt_list               { drv.ast = std::move($1); }
    ;

stmt_list:
    stmt                    { $$ = std::vector<std::unique_ptr<ExprAST>>(); $$.push_back(std::move($1)); }
    | stmt_list stmt        { $$.swap($1); $$.push_back(std::move($2)); }
    |                       { $$ = std::vector<std::unique_ptr<ExprAST>>(); }
    ;

stmt:
    DEF proto expr          { $$ = std::make_unique<FunctionAST>(std::move($2), std::move($3)); }
    | EXTERN proto          { $$ = std::move($2); }
    | expr                  { $$ = std::make_unique<FunctionAST>(std::make_unique<PrototypeAST>("__anon_expr", std::vector<std::string>()), std::move($1));}
    ;

%left "+" "-";
%left "*" "/";
expr:
    "(" expr ")"            { $$ = std::move($2); }
    | expr "+" expr         { $$ = std::make_unique<BinaryExprAST>('+', std::move($1), std::move($3)); }
    | expr "-" expr         { $$ = std::make_unique<BinaryExprAST>('-', std::move($1), std::move($3)); }
    | expr "*" expr         { $$ = std::make_unique<BinaryExprAST>('*', std::move($1), std::move($3)); }
    | expr "/" expr         { $$ = std::make_unique<BinaryExprAST>('/', std::move($1), std::move($3)); }
    | NUM                   { $$ = std::make_unique<NumberExprAST>(std::move($1)); }
    | ID                    { $$ = std::make_unique<VariableExprAST>($1); }
    | ID "(" expr_list ")"  { $$ = std::make_unique<CallExprAST>($1, std::move($3)); }
    ;

expr_list:
    expr                    { $$ = std::vector<std::unique_ptr<ExprAST>>(); $$.push_back(std::move($1)); }
    | expr_list "," expr    { $$.swap($1); $$.push_back(std::move($3)); }
    |                       { $$ = std::vector<std::unique_ptr<ExprAST>>(); }
    ;

id_list:
    ID                      { $$ = std::vector<std::string>{$1}; }
    | id_list ID            { $$.swap($1); $$.push_back($2); }
    |                       { $$ = std::vector<std::string>(); }
    ;

proto:
    ID "(" id_list ")"      { $$ = std::make_unique<PrototypeAST>($1, $3); }
    ;
%%

void yy::parser::error (const location_type& l, const std::string& m)
{
  std::cerr << l << ": " << m << '\n';
}
