#pragma once

#include <map>
#include "ast.hh"

#include "llvm/Support/Error.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/DIBuilder.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LegacyPassManager.h"

class CodeGenVisitor : public ExprASTVisitor
{
    std::unique_ptr<llvm::IRBuilder<>> Builder;
    std::map<std::string, llvm::AllocaInst *> NamedValues;
    std::unique_ptr<llvm::legacy::FunctionPassManager> TheFPM;

    std::map<std::string, const PrototypeAST &> FunctionProtos_;

    llvm::Function *getFunction(const std::string &Name);
    llvm::AllocaInst *CreateEntryBlockAlloca(llvm::Function *TheFunction, const std::string &VarName);

    bool debug_;
    std::unique_ptr<llvm::DIBuilder> DBuilder;

    struct DebugInfo
    {
        llvm::DICompileUnit *TheCU;
        llvm::DIType *DblTy;
        std::vector<llvm::DIScope *> LexicalBlocks;
    } KSDbgInfo;

    llvm::DIType *getDoubleTy();
    void emitLocation(const ExprAST *AST);
    llvm::DISubroutineType *CreateFunctionType(unsigned NumArgs);

public:
    CodeGenVisitor(bool debug = false)
        : debug_(debug)
    {
        InitializeModuleAndPassManager();
    }

    virtual void visit(const ExprAST &e) { e.accept(*this); }
    virtual void visit(const NumberExprAST &n) override;
    virtual void visit(const VariableExprAST &v) override;
    virtual void visit(const BinaryExprAST &b) override;
    virtual void visit(const CallExprAST &c) override;
    virtual void visit(const PrototypeAST &p) override;
    virtual void visit(const FunctionAST &f) override;
    virtual void visit(const IfExprAST &i) override;
    virtual void visit(const ForExprAST &f) override;
    virtual void visit(const AssignExprAST &f) override;
    virtual void visit(const VarExprAST &a) override;

    void dump() const
    {
        if (debug_)
            DBuilder->finalize();
        TheModule->print(llvm::errs(), nullptr);
    }

protected:
    std::unique_ptr<llvm::LLVMContext> TheContext;
    std::unique_ptr<llvm::Module> TheModule;
    llvm::ExitOnError ExitOnErr;

    llvm::Value *Value_;

    void InitializeModuleAndPassManager();
};
