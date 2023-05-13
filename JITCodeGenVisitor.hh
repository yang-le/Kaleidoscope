#pragma once

#include "CodeGenVisitor.hh"
#include "KaleidoscopeJIT.h"

class JITCodeGenVisitor : public CodeGenVisitor
{
    std::unique_ptr<llvm::orc::KaleidoscopeJIT> TheJIT;

    void InitializeJIT();

public:
    JITCodeGenVisitor() : CodeGenVisitor()
    {
        InitializeJIT();
    }

    void visit(const FunctionAST &f) override;
    using CodeGenVisitor::visit;
};
