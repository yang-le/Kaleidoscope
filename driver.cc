#include "driver.hh"
#include "parser.hh"

driver::driver()
    : trace_parsing(false), trace_scanning(false)
{
    InitializeJIT();
    InitializeModuleAndPassManager();
}

int driver::parse(const std::string &f)
{
    file = f;
    location.initialize(&file);
    scan_begin();
    yy::parser parse(*this);
    parse.set_debug_level(trace_parsing);
    int res = parse();
    scan_end();
    return res;
}

void driver::codegen()
{
    for (auto &expr : ast)
        expr->codegen();
}
