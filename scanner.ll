%{ /* -*- C++ -*- */
#include "driver.hh"
#include "parser.hh"
%}

%option noyywrap nounput noinput batch debug

id      [[:alpha:]][[:alnum:]_]*
num     [[:digit:].]+
blank   [[:blank:]\r]

%{
    // Code run each time a pattern is matched.
    # define YY_USER_ACTION  loc.columns(yyleng);
%}
%%
%{
    // A handy shortcut to the location held by the driver.
    yy::location& loc = drv.location;
    // Code run each time yylex is called.
    loc.step();
%}
{blank}+                loc.step();
\n+                     loc.lines(yyleng); loc.step();

"def"                   return yy::parser::make_DEF(loc);
"extern"                return yy::parser::make_EXTERN(loc);
"if"                    return yy::parser::make_IF(loc);
"then"                  return yy::parser::make_THEN(loc);
"else"                  return yy::parser::make_ELSE(loc);
"for"                   return yy::parser::make_FOR(loc);
"in"                    return yy::parser::make_IN(loc);
"unary"                 return yy::parser::make_UNARY(loc);
"binary"                return yy::parser::make_BINARY(loc);
"var"                   return yy::parser::make_VAR(loc);
{id}                    return yy::parser::make_ID(yytext, loc);
{num}                   return yy::parser::make_NUM(atof(yytext), loc);
#.*$                    /* comment */
";"                     /* ignore */
"-"                     return yy::parser::make_MINUS(loc);
"+"                     return yy::parser::make_PLUS(loc);
"*"                     return yy::parser::make_STAR(loc);
"/"                     return yy::parser::make_SLASH(loc);
"("                     return yy::parser::make_LPAREN(loc);
")"                     return yy::parser::make_RPAREN(loc);
"="                     return yy::parser::make_ASSIGN(loc);
","                     return yy::parser::make_COMMA(loc);
"<"                     return yy::parser::make_LT(loc);
[[:punct:]]             return yy::parser::make_PUNCT(yytext[0], loc);
.                       std::cerr << loc << "invalid character: " << yytext << std::endl;
<<EOF>>                 return yy::parser::make_END(loc);
%%

void driver::scan_begin()
{
    yy_flex_debug = trace_scanning;
    if (file.empty () || file == "-")
        yyin = stdin;
    else if (!(yyin = fopen(file.c_str(), "r"))) {
        std::cerr << "cannot open " << file << ": " << strerror(errno) << '\n';
        exit(EXIT_FAILURE);
    }
}

void driver::scan_end()
{
    fclose (yyin);
}
