#include "CodeGenVisitor.hh"

#include <iostream>

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
#include "llvm/Target/TargetMachine.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/Transforms/Utils.h"

using namespace llvm;

void CodeGenVisitor::visit(const NumberExprAST &n)
{
    Value_ = ConstantFP::get(*TheContext, APFloat(n.getVal()));
}

void CodeGenVisitor::visit(const VariableExprAST &v)
{
    AllocaInst *A = NamedValues[v.getName()];
    if (!A)
    {
        std::cerr << "Unknown variable name: " << v.getName() << '\n';
        Value_ = nullptr;
        return;
    }

    Value_ = Builder->CreateLoad(A->getAllocatedType(), A, v.getName());
}

void CodeGenVisitor::visit(const BinaryExprAST &b)
{
    visit(b.getLHS());
    Value *L = Value_;

    visit(b.getRHS());
    Value *R = Value_;

    if (!L || !R)
    {
        Value_ = nullptr;
        return;
    }

    switch (b.getOp())
    {
    case '+':
        Value_ = Builder->CreateFAdd(L, R, "fadd");
        return;
    case '-':
        Value_ = Builder->CreateFSub(L, R, "fsub");
        return;
    case '*':
        Value_ = Builder->CreateFMul(L, R, "fmul");
        return;
    case '/':
        Value_ = Builder->CreateFDiv(L, R, "fdiv");
        return;
    case '<':
        Value_ = Builder->CreateUIToFP(Builder->CreateFCmpULT(L, R, "flt"), Type::getDoubleTy(*TheContext), "bool");
        return;
    default:
        break;
    }

    Function *F = getFunction(std::string("binary") + b.getOp());
    assert(F && "binary operator not found!");

    Value_ = Builder->CreateCall(F, {L, R}, "binop");
}

void CodeGenVisitor::visit(const CallExprAST &c)
{
    Function *CalleeF = getFunction(c.getCallee());
    if (!CalleeF)
    {
        std::cerr << "Unknown function: " << c.getCallee() << '\n';
        Value_ = nullptr;
        return;
    }

    if (CalleeF->arg_size() != c.getArgsSize())
    {
        std::cerr << c.getArgsSize() << " arguments passed, expect " << CalleeF->arg_size() << '\n';
        Value_ = nullptr;
        return;
    }

    std::vector<Value *> ArgsV;
    for (unsigned i = 0, e = c.getArgsSize(); i < e; ++i)
    {
        visit(c.getArgs(i));
        if (!Value_)
            return;
        ArgsV.push_back(Value_);
    }

    Value_ = Builder->CreateCall(CalleeF, ArgsV, "call");
}

void CodeGenVisitor::visit(const PrototypeAST &p)
{
    std::vector<Type *> Doubles(p.getArgsSize(), Type::getDoubleTy(*TheContext));
    FunctionType *FT = FunctionType::get(Type::getDoubleTy(*TheContext), Doubles, false);
    Function *F = Function::Create(FT, Function::ExternalLinkage, p.getName(), *TheModule);

    unsigned Idx = 0;
    for (auto &Arg : F->args())
        Arg.setName(p.getArgs(Idx++));

    FunctionProtos_.emplace(p.getName(), p);

    Value_ = F;
}

void CodeGenVisitor::visit(const FunctionAST &f)
{
    auto &Proto = f.getProto();
    FunctionProtos_.emplace(Proto.getName(), Proto);

    Function *TheFunction = getFunction(Proto.getName());
    if (!TheFunction)
    {
        Value_ = nullptr;
        return;
    }

    BasicBlock *BB = BasicBlock::Create(*TheContext, "entry", TheFunction);
    Builder->SetInsertPoint(BB);

    NamedValues.clear();
    for (auto &Arg : TheFunction->args())
    {
        AllocaInst *Alloca = CreateEntryBlockAlloca(TheFunction, std::string(Arg.getName()));
        Builder->CreateStore(&Arg, Alloca);
        NamedValues[std::string(Arg.getName())] = Alloca;
    }

    visit(f.getBody());
    if (Value_)
    {
        Builder->CreateRet(Value_);
        verifyFunction(*TheFunction);
        TheFPM->run(*TheFunction);

        Value_ = TheFunction;
        return;
    }

    TheFunction->eraseFromParent();
    Value_ = nullptr;
}

void CodeGenVisitor::visit(const IfExprAST &i)
{
    visit(i.getCond());
    Value *CondV = Value_;
    if (!CondV)
        return;

    CondV = Builder->CreateFCmpONE(CondV, ConstantFP::get(*TheContext, APFloat(0.0)), "ifcond");

    Function *TheFunction = Builder->GetInsertBlock()->getParent();

    BasicBlock *ThenBB = BasicBlock::Create(*TheContext, "then", TheFunction);
    BasicBlock *ElseBB = BasicBlock::Create(*TheContext, "else");
    BasicBlock *MergeBB = BasicBlock::Create(*TheContext, "ifcont");

    Builder->CreateCondBr(CondV, ThenBB, ElseBB);

    Builder->SetInsertPoint(ThenBB);

    visit(i.getThen());
    Value *ThenV = Value_;
    if (!ThenV)
        return;

    Builder->CreateBr(MergeBB);
    ThenBB = Builder->GetInsertBlock();

    TheFunction->getBasicBlockList().push_back(ElseBB);
    Builder->SetInsertPoint(ElseBB);

    visit(i.getElse());
    Value *ElseV = Value_;
    if (!ElseV)
        return;

    Builder->CreateBr(MergeBB);
    ElseBB = Builder->GetInsertBlock();

    TheFunction->getBasicBlockList().push_back(MergeBB);
    Builder->SetInsertPoint(MergeBB);

    PHINode *PN = Builder->CreatePHI(Type::getDoubleTy(*TheContext), 2, "if");
    PN->addIncoming(ThenV, ThenBB);
    PN->addIncoming(ElseV, ElseBB);
    Value_ = PN;
}

void CodeGenVisitor::visit(const ForExprAST &f)
{
    Function *TheFunction = Builder->GetInsertBlock()->getParent();
    AllocaInst *Alloca = CreateEntryBlockAlloca(TheFunction, f.getVarName());

    visit(f.getStart());
    Value *StartVal = Value_;
    if (!StartVal)
        return;

    Builder->CreateStore(StartVal, Alloca);

    BasicBlock *LoopBB = BasicBlock::Create(*TheContext, "loop", TheFunction);
    Builder->CreateBr(LoopBB);
    Builder->SetInsertPoint(LoopBB);

    AllocaInst *OldVal = NamedValues[f.getVarName()];
    NamedValues[f.getVarName()] = Alloca;

    visit(f.getBody());
    if (!Value_)
        return;

    Value *StepVal;
    if (f.isStepValid())
    {
        visit(f.getStep());
        StepVal = Value_;

        if (!StepVal)
            return;
    }
    else
    {
        StepVal = ConstantFP::get(*TheContext, APFloat(1.0));
    }

    visit(f.getEnd());
    Value *EndCond = Value_;
    if (!EndCond)
        return;

    Value *CurVar = Builder->CreateLoad(Alloca->getAllocatedType(), Alloca, f.getVarName());
    Value *NextVar = Builder->CreateFAdd(CurVar, StepVal, "nextvar");
    Builder->CreateStore(NextVar, Alloca);

    EndCond = Builder->CreateFCmpONE(EndCond, ConstantFP::get(*TheContext, APFloat(0.0)), "loopcond");

    BasicBlock *LoopEndBB = Builder->GetInsertBlock();
    BasicBlock *AfterBB = BasicBlock::Create(*TheContext, "afterloop", TheFunction);

    Builder->CreateCondBr(EndCond, LoopBB, AfterBB);

    Builder->SetInsertPoint(AfterBB);

    if (OldVal)
        NamedValues[f.getVarName()] = OldVal;
    else
        NamedValues.erase(f.getVarName());

    Value_ = Constant::getNullValue(Type::getDoubleTy(*TheContext));
}

void CodeGenVisitor::visit(const AssignExprAST &f)
{
    visit(f.getRHS());
    if (!Value_)
        return;

    AllocaInst *A = NamedValues[f.getVarName()];
    if (!A)
        std::cerr << "Unknown variable name: " << f.getVarName() << '\n';

    Builder->CreateStore(Value_, A);

    Value_ = A;
}

void CodeGenVisitor::visit(const VarExprAST &a)
{
    std::vector<AllocaInst *> OldBindings;

    Function *TheFunction = Builder->GetInsertBlock()->getParent();

    for (auto &[VarName, Init] : a.getVars())
    {
        if (Init)
        {
            visit(*Init);
            if (!Value_)
                return;
        }
        else
        {
            Value_ = ConstantFP::get(*TheContext, APFloat(0.0));
        }

        AllocaInst *Alloca = CreateEntryBlockAlloca(TheFunction, VarName);
        Builder->CreateStore(Value_, Alloca);

        OldBindings.push_back(NamedValues[VarName]);
        NamedValues[VarName] = Alloca;
    }

    visit(a.getBody());
    if (!Value_)
        return;

    unsigned i = 0;
    for (auto &[VarName, _] : a.getVars())
        NamedValues[VarName] = OldBindings[i++];
}

Function *CodeGenVisitor::getFunction(const std::string &Name)
{
    if (auto *F = TheModule->getFunction(Name))
        return F;

    auto FI = FunctionProtos_.find(Name);
    if (FI != FunctionProtos_.end())
    {
        Value *temp = Value_;
        visit(FI->second);
        std::swap(temp, Value_);
        return (Function *)temp;
    }

    return nullptr;
}

AllocaInst *CodeGenVisitor::CreateEntryBlockAlloca(Function *TheFunction, const std::string &VarName)
{
    IRBuilder<> TmpB(&TheFunction->getEntryBlock(), TheFunction->getEntryBlock().begin());
    return TmpB.CreateAlloca(Type::getDoubleTy(*TheContext), nullptr, VarName);
}

void CodeGenVisitor::InitializeModuleAndPassManager()
{
    TheContext = std::make_unique<LLVMContext>();
    TheModule = std::make_unique<Module>("My Cool JIT", *TheContext);

    Builder = std::make_unique<IRBuilder<>>(*TheContext);

    TheFPM = std::make_unique<legacy::FunctionPassManager>(TheModule.get());

    TheFPM->add(createPromoteMemoryToRegisterPass());
    TheFPM->add(createInstructionCombiningPass());
    TheFPM->add(createReassociatePass());
    TheFPM->add(createGVNPass());
    TheFPM->add(createCFGSimplificationPass());

    TheFPM->doInitialization();
}
