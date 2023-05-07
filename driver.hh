#pragma once

#include <string>
#include <map>
#include "parser.hh"
#include "ast.hh"

#define YY_DECL \
    yy::parser::symbol_type yylex(driver &drv)

YY_DECL;

class driver
{
public:
    driver();

    std::vector<std::unique_ptr<ExprAST>> ast;

    int parse(const std::string &f);

    std::string file;
    bool trace_parsing;

    void scan_begin();
    void scan_end();

    bool trace_scanning;
    yy::location location;

    void codegen();
};
