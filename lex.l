%{
#include <string>
#include <memory>
#include "ast.h"
#include "y.tab.h"
void yyerror(const char* msg);
%}

%%
"def"                   { return DEF; }
"extern"                { return EXTERN; }
[a-zA-Z][a-zA-Z0-9]*    { yylval.id = new std::string(yytext); return ID; }
[0-9.]+                 { yylval.num = atof(yytext); return NUM; }
[[:space:]]             { /* whitespace */ }
#.*$                    { /* comment */ }
[-+*/,()]               { return yytext[0]; }
.                       { yyerror("invalid input"); }
%%

int yywrap(void)
{
    return 1;
}
