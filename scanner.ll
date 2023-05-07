%{ /* -*- C++ -*- */
#include "driver.hh"
#include "parser.hh"
%}

%option noyywrap nounput noinput batch debug

id      [a-zA-Z][a-zA-Z_0-9]*
num     [0-9.]+

%{
    // Code run each time a pattern is matched.
    # define YY_USER_ACTION  loc.columns(yyleng);
%}
%%
%{
    // A handy shortcut to the location held by the driver.
    yy::location& loc = drv.location;
    // Code run each time yylex is called.
    loc.step ();
%}
"def"                   return yy::parser::make_DEF(loc);
"extern"                return yy::parser::make_EXTERN(loc);
{id}                    return yy::parser::make_ID(yytext, loc);
{num}                   return yy::parser::make_NUM(atof(yytext), loc);
[[:space:]]             loc.step ();
\n+                     loc.lines (yyleng); loc.step ();
#.*$                    /* comment */
";"                     /* ignore */
"-"                     return yy::parser::make_MINUS  (loc);
"+"                     return yy::parser::make_PLUS   (loc);
"*"                     return yy::parser::make_STAR   (loc);
"/"                     return yy::parser::make_SLASH  (loc);
"("                     return yy::parser::make_LPAREN (loc);
")"                     return yy::parser::make_RPAREN (loc);
"="                     return yy::parser::make_ASSIGN (loc);
","                     return yy::parser::make_COMMA  (loc);
.                       {
                            throw yy::parser::syntax_error
                            (loc, "invalid character: " + std::string(yytext));
}
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
