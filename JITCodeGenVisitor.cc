#include "JITCodeGenVisitor.hh"

#include <iostream>

#include "llvm/Support/TargetSelect.h"

using namespace llvm;
using namespace llvm::orc;

void JITCodeGenVisitor::visit(const FunctionAST &f)
{
    CodeGenVisitor::visit(f);
    if (Value_) {
        auto RT = TheJIT->getMainJITDylib().createResourceTracker();
        auto TSM = ThreadSafeModule(std::move(TheModule), std::move(TheContext));
        ExitOnErr(TheJIT->addModule(std::move(TSM), RT));
        InitializeModuleAndPassManager();
        TheModule->setDataLayout(TheJIT->getDataLayout());

        if ("__anon_expr" == f.getProto().getName())
        {
            auto ExprSymbol = ExitOnErr(TheJIT->lookup("__anon_expr"));
            assert(ExprSymbol && "Function not found");

            double (*FP)() = (double (*)())(intptr_t)ExprSymbol.getAddress();
            std::cout << "Evaluated to " << FP() << '\n';

            ExitOnErr(RT->remove());
        }
    }
}

void JITCodeGenVisitor::InitializeJIT()
{
    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();
    InitializeNativeTargetAsmParser();

    TheJIT = ExitOnErr(KaleidoscopeJIT::Create());
    TheModule->setDataLayout(TheJIT->getDataLayout());
}
