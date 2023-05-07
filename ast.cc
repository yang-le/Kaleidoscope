#include <iostream>
#include "ast.hh"

namespace
{
    LLVMContext TheContext;
    IRBuilder<> Builder(TheContext);
    Module TheModule("My Cool JIT", TheContext);
    std::map<std::string, Value *> NamedValues;
}

Value *NumberExprAST::codegen()
{
    return ConstantFP::get(TheContext, APFloat(Val));
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
        return Builder.CreateFAdd(L, R, "fadd");
    case '-':
        return Builder.CreateFSub(L, R, "fsub");
    case '*':
        return Builder.CreateFMul(L, R, "fmul");
    case '/':
        return Builder.CreateFDiv(L, R, "fdiv");
    default:
        std::cerr << "Invalid binary operator" << Op << '\n';
        return nullptr;
    }
}

Value *CallExprAST::codegen()
{
    Function *CalleeF = TheModule.getFunction(Callee);
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

    return Builder.CreateCall(CalleeF, ArgsV, "call");
}

Value *PrototypeAST::codegen()
{
    std::vector<Type *> Doubles(Args.size(), Type::getDoubleTy(TheContext));
    FunctionType *FT = FunctionType::get(Type::getDoubleTy(TheContext), Doubles, false);
    Function *F = Function::Create(FT, Function::ExternalLinkage, Name, TheModule);

    unsigned Idx = 0;
    for (auto &Arg : F->args())
        Arg.setName(Args[Idx++]);

    return F;
}

Value *FunctionAST::codegen()
{
    Function *TheFunction = TheModule.getFunction(Proto->getName());

    if (!TheFunction)
        TheFunction = static_cast<Function *>(Proto->codegen());

    if (!TheFunction)
        return nullptr;

    if (!TheFunction->empty())
    {
        std::cerr << Proto->getName() << " Function cannot be redefined\n";
        return nullptr;
    }

    BasicBlock *BB = BasicBlock::Create(TheContext, "entry", TheFunction);
    Builder.SetInsertPoint(BB);

    NamedValues.clear();
    for (auto &Arg : TheFunction->args())
        NamedValues[std::string(Arg.getName())] = &Arg;

    if (Value *RetVal = Body->codegen())
    {
        Builder.CreateRet(RetVal);
        verifyFunction(*TheFunction);
        return TheFunction;
    }

    TheFunction->eraseFromParent();
    return nullptr;
}

void codedump()
{
    TheModule.print(errs(), nullptr);
}
