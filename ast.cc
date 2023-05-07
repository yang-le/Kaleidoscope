#include "ast.hh"

#include <iostream>

#include "KaleidoscopeJIT.h"
#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"

using namespace llvm;
using namespace llvm::orc;

namespace
{
    std::unique_ptr<LLVMContext> TheContext;
    std::unique_ptr<Module> TheModule;
    std::unique_ptr<IRBuilder<>> Builder;
    std::map<std::string, Value *> NamedValues;
    std::unique_ptr<legacy::FunctionPassManager> TheFPM;
    std::unique_ptr<KaleidoscopeJIT> TheJIT;
    std::map<std::string, std::unique_ptr<PrototypeAST>> FunctionProtos;
    ExitOnError ExitOnErr;
}

Value *NumberExprAST::codegen()
{
    return ConstantFP::get(*TheContext, APFloat(Val));
}

Value *VariableExprAST::codegen()
{
    Value *V = NamedValues[Name];
    if (!V)
    {
        std::cerr << "Unknown variable name: " << Name << '\n';
        return nullptr;
    }
    return V;
}

Value *BinaryExprAST::codegen()
{
    Value *L = LHS->codegen();
    Value *R = RHS->codegen();

    if (!L || !R)
        return nullptr;

    switch (Op)
    {
    case '+':
        return Builder->CreateFAdd(L, R, "fadd");
    case '-':
        return Builder->CreateFSub(L, R, "fsub");
    case '*':
        return Builder->CreateFMul(L, R, "fmul");
    case '/':
        return Builder->CreateFDiv(L, R, "fdiv");
    default:
        std::cerr << "Invalid binary operator" << Op << '\n';
        return nullptr;
    }
}

Function *getFunction(const std::string &Name)
{
    if (auto *F = TheModule->getFunction(Name))
        return F;

    auto FI = FunctionProtos.find(Name);
    if (FI != FunctionProtos.end())
        return (Function *)FI->second->codegen();

    return nullptr;
}

Value *CallExprAST::codegen()
{
    Function *CalleeF = getFunction(Callee);
    if (!CalleeF)
    {
        std::cerr << "Unknown function: " << Callee << '\n';
        return nullptr;
    }

    if (CalleeF->arg_size() != Args.size())
    {
        std::cerr << Args.size() << " arguments passed, expect " << CalleeF->arg_size() << '\n';
        return nullptr;
    }

    std::vector<Value *> ArgsV;
    for (unsigned i = 0, e = Args.size(); i < e; ++i)
    {
        Value *V = Args[i]->codegen();
        if (!V)
            return nullptr;
        ArgsV.push_back(V);
    }

    return Builder->CreateCall(CalleeF, ArgsV, "call");
}

Value *PrototypeAST::codegen()
{
    std::vector<Type *> Doubles(Args.size(), Type::getDoubleTy(*TheContext));
    FunctionType *FT = FunctionType::get(Type::getDoubleTy(*TheContext), Doubles, false);
    Function *F = Function::Create(FT, Function::ExternalLinkage, Name, *TheModule);

    unsigned Idx = 0;
    for (auto &Arg : F->args())
        Arg.setName(Args[Idx++]);
    
    F->print(errs());

    return F;
}

Value *FunctionAST::codegen()
{
    auto &P = *Proto;
    FunctionProtos[Proto->getName()] = std::move(Proto);

    Function *TheFunction = getFunction(P.getName());
    if (!TheFunction)
        return nullptr;

    BasicBlock *BB = BasicBlock::Create(*TheContext, "entry", TheFunction);
    Builder->SetInsertPoint(BB);

    NamedValues.clear();
    for (auto &Arg : TheFunction->args())
        NamedValues[std::string(Arg.getName())] = &Arg;

    if (Value *RetVal = Body->codegen())
    {
        Builder->CreateRet(RetVal);
        verifyFunction(*TheFunction);
        TheFPM->run(*TheFunction);

        TheFunction->print(errs());

        auto RT = TheJIT->getMainJITDylib().createResourceTracker();
        auto TSM = ThreadSafeModule(std::move(TheModule), std::move(TheContext));
        ExitOnErr(TheJIT->addModule(std::move(TSM), RT));
        InitializeModuleAndPassManager();

        if ("__anon_expr" == P.getName())
        {
            auto ExprSymbol = ExitOnErr(TheJIT->lookup("__anon_expr"));
            assert(ExprSymbol && "Function not found");

            double (*FP)() = (double (*)())(intptr_t)ExprSymbol.getAddress();
            std::cout << "Evaluated to " << FP() << '\n';

            ExitOnErr(RT->remove());
        }

        return TheFunction;
    }

    TheFunction->eraseFromParent();
    return nullptr;
}

void InitializeModuleAndPassManager()
{
    TheContext = std::make_unique<LLVMContext>();
    TheModule = std::make_unique<Module>("My Cool JIT", *TheContext);
    TheModule->setDataLayout(TheJIT->getDataLayout());

    Builder = std::make_unique<IRBuilder<>>(*TheContext);

    TheFPM = std::make_unique<legacy::FunctionPassManager>(TheModule.get());

    TheFPM->add(createInstructionCombiningPass());
    TheFPM->add(createReassociatePass());
    TheFPM->add(createGVNPass());
    TheFPM->add(createCFGSimplificationPass());

    TheFPM->doInitialization();
}

void InitializeJIT()
{
    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();
    InitializeNativeTargetAsmParser();

    TheJIT = ExitOnErr(KaleidoscopeJIT::Create());
}
