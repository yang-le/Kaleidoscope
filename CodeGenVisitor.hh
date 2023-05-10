#pragma once

#include <map>
#include "ast.hh"

#include "KaleidoscopeJIT.h"

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LegacyPassManager.h"

class CodeGenVisitor : public ExprASTVisitor
{
    std::unique_ptr<llvm::LLVMContext> TheContext;
    std::unique_ptr<llvm::Module> TheModule;
    std::unique_ptr<llvm::IRBuilder<>> Builder;
    std::map<std::string, llvm::Value *> NamedValues;
    std::unique_ptr<llvm::legacy::FunctionPassManager> TheFPM;
    std::unique_ptr<llvm::orc::KaleidoscopeJIT> TheJIT;
    llvm::ExitOnError ExitOnErr;

    llvm::Value *Value_;
    std::map<std::string, const PrototypeAST &> FunctionProtos_;

    llvm::Function *getFunction(const std::string &Name);

    void InitializeModuleAndPassManager();
    void InitializeJIT();

public:
    CodeGenVisitor()
    {
        InitializeJIT();
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
};
