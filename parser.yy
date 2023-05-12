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
  LT      "<"
  DEF     "def"
  EXTERN  "extern"
  IF      "if"
  THEN    "then"
  ELSE    "else"
  FOR     "for"
  IN      "in"
  UNARY   "unary"
  BINARY  "binary"
  VAR     "var"

%token <std::string> ID "identifier"
%token <double> NUM "number"
%token <char> PUNCT "punct"

%nterm <std::unique_ptr<ExprAST>> expr stmt
%nterm <std::unique_ptr<PrototypeAST>> proto
%nterm <std::vector<std::unique_ptr<ExprAST>>> expr_list stmt_list
%nterm <std::vector<std::string>> id_list
%nterm <std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>>> var_list;

/* %printer { yyo << $$; } <*> */

%%
%start prog;
prog:
    stmt_list                                       { drv.ast = std::move($1); }
    ;

stmt_list:
    stmt                                            { $$ = std::vector<std::unique_ptr<ExprAST>>(); $$.push_back(std::move($1)); }
    | stmt_list stmt                                { $$.swap($1); $$.push_back(std::move($2)); }
    |                                               { $$ = std::vector<std::unique_ptr<ExprAST>>(); }
    ;

stmt:
    DEF proto expr                                  { $$ = std::make_unique<FunctionAST>(std::move($2), std::move($3)); }
    | EXTERN proto                                  { $$ = std::move($2); }
    | expr                                          { $$ = std::make_unique<FunctionAST>(std::make_unique<PrototypeAST>("__anon_expr", std::vector<std::string>()), std::move($1));}
    ;

%left "<";
%left "+" "-";
%left "*" "/";
expr:
    "(" expr ")"                                    { $$ = std::move($2); }
    | expr "+" expr                                 { $$ = std::make_unique<BinaryExprAST>('+', std::move($1), std::move($3)); }
    | expr "-" expr                                 { $$ = std::make_unique<BinaryExprAST>('-', std::move($1), std::move($3)); }
    | expr "*" expr                                 { $$ = std::make_unique<BinaryExprAST>('*', std::move($1), std::move($3)); }
    | expr "/" expr                                 { $$ = std::make_unique<BinaryExprAST>('/', std::move($1), std::move($3)); }
    | expr "<" expr                                 { $$ = std::make_unique<BinaryExprAST>('<', std::move($1), std::move($3)); }
    | expr PUNCT expr                               { $$ = std::make_unique<BinaryExprAST>($2, std::move($1), std::move($3)); }
    | NUM                                           { $$ = std::make_unique<NumberExprAST>(std::move($1)); }
    | ID                                            { $$ = std::make_unique<VariableExprAST>($1); }
    | ID "(" expr_list ")"                          { $$ = std::make_unique<CallExprAST>($1, std::move($3)); }
    | IF expr THEN expr ELSE expr                   { $$ = std::make_unique<IfExprAST>(std::move($2), std::move($4), std::move($6)); }
    | FOR ID "=" expr "," expr IN expr              { $$ = std::make_unique<ForExprAST>($2, std::move($4), std::move($6), nullptr, std::move($8)); }
    | FOR ID "=" expr "," expr "," expr IN expr     { $$ = std::make_unique<ForExprAST>($2, std::move($4), std::move($6), std::move($8), std::move($10)); }
    | ID "=" expr                                   { $$ = std::make_unique<AssignExprAST>($1, std::move($3)); }
    | VAR var_list IN expr                          { $$ = std::make_unique<VarExprAST>(std::move($2), std::move($4)); }
    ;

var_list:
    ID                                              { $$ = std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>>(); $$.emplace_back($1, nullptr); }
    | ID "=" expr                                   { $$ = std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>>(); $$.emplace_back($1, std::move($3)); }
    | var_list "," ID                               { $$.swap($1); $$.emplace_back($3, nullptr); }
    | var_list "," ID "=" expr                      { $$.swap($1); $$.emplace_back($3, std::move($5)); }
    ;

expr_list:
    expr                                            { $$ = std::vector<std::unique_ptr<ExprAST>>(); $$.push_back(std::move($1)); }
    | expr_list "," expr                            { $$.swap($1); $$.push_back(std::move($3)); }
    |                                               { $$ = std::vector<std::unique_ptr<ExprAST>>(); }
    ;

id_list:
    ID                                              { $$ = std::vector<std::string>{$1}; }
    | id_list ID                                    { $$.swap($1); $$.push_back($2); }
    |                                               { $$ = std::vector<std::string>(); }
    ;

proto:
    ID "(" id_list ")"                              { $$ = std::make_unique<PrototypeAST>($1, std::move($3)); }
    | BINARY PUNCT "(" ID ID ")"                    { $$ = std::make_unique<PrototypeAST>(std::string("binary") + $2, std::vector<std::string>{$4, $5}, true); }
    | UNARY PUNCT "(" ID ")"                        { $$ = std::make_unique<PrototypeAST>(std::string("unary") + $2, std::vector<std::string>{$4}, true); }
    ;
%%

void yy::parser::error (const location_type& l, const std::string& m)
{
  std::cerr << l << ": " << m << '\n';
}
