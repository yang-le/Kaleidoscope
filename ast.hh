#pragma once

#include <cassert>
#include <string>
#include <vector>
#include <memory>

class ExprAST;
class NumberExprAST;
class VariableExprAST;
class BinaryExprAST;
class CallExprAST;
class PrototypeAST;
class FunctionAST;
class IfExprAST;
class ForExprAST;

class ExprASTVisitor
{
public:
    virtual void visit(const ExprAST &e) = 0;
    virtual void visit(const NumberExprAST &n) = 0;
    virtual void visit(const VariableExprAST &v) = 0;
    virtual void visit(const BinaryExprAST &b) = 0;
    virtual void visit(const CallExprAST &c) = 0;
    virtual void visit(const PrototypeAST &p) = 0;
    virtual void visit(const FunctionAST &f) = 0;
    virtual void visit(const IfExprAST &i) = 0;
    virtual void visit(const ForExprAST &f) = 0;
};

class ExprAST
{
public:
    virtual ~ExprAST() = default;

    virtual void accept(ExprASTVisitor &v) const = 0;
};

class NumberExprAST : public ExprAST
{
    double Val;

public:
    NumberExprAST(double Val) : Val(Val) {}

    double getVal() const { return Val; }

    virtual void accept(ExprASTVisitor &v) const override { v.visit(*this); }
};

class VariableExprAST : public ExprAST
{
    std::string Name;

public:
    VariableExprAST(const std::string &Name) : Name(Name) {}

    const std::string &getName() const { return Name; }

    virtual void accept(ExprASTVisitor &v) const override { v.visit(*this); }
};

class BinaryExprAST : public ExprAST
{
    char Op;
    std::unique_ptr<ExprAST> LHS, RHS;

public:
    BinaryExprAST(char Op, std::unique_ptr<ExprAST> LHS, std::unique_ptr<ExprAST> RHS)
        : Op(Op), LHS(std::move(LHS)), RHS(std::move(RHS)) {}

    char getOp() const { return Op; }
    const ExprAST &getLHS() const { return *LHS; }
    const ExprAST &getRHS() const { return *RHS; }

    virtual void accept(ExprASTVisitor &v) const override { v.visit(*this); }
};

class CallExprAST : public ExprAST
{
    std::string Callee;
    std::vector<std::unique_ptr<ExprAST>> Args;

public:
    CallExprAST(const std::string &Callee, std::vector<std::unique_ptr<ExprAST>> Args)
        : Callee(Callee), Args(std::move(Args)) {}

    const std::string &getCallee() const { return Callee; }
    const ExprAST &getArgs(std::size_t i) const { return *Args[i]; }
    size_t getArgsSize() const { return Args.size(); }

    virtual void accept(ExprASTVisitor &v) const override { v.visit(*this); }
};

class PrototypeAST : public ExprAST
{
    std::string Name;
    std::vector<std::string> Args;
    bool IsOperator;

public:
    PrototypeAST(const std::string &Name, std::vector<std::string> Args, bool IsOperator = false)
        : Name(Name), Args(std::move(Args)), IsOperator(IsOperator) {}

    const std::string &getName() const { return Name; }
    const std::string &getArgs(std::size_t i) const { return Args[i]; }
    size_t getArgsSize() const { return Args.size(); }

    bool isUnaryOp() const { return IsOperator && Args.size() == 1; }
    bool isBinaryOp() const { return IsOperator && Args.size() == 2; }

    char getOperatorName() const
    {
        assert(isUnaryOp() || isBinaryOp());
        return Name[Name.size() - 1];
    }

    virtual void accept(ExprASTVisitor &v) const override { v.visit(*this); }
};

class FunctionAST : public ExprAST
{
    std::unique_ptr<PrototypeAST> Proto;
    std::unique_ptr<ExprAST> Body;

public:
    FunctionAST(std::unique_ptr<PrototypeAST> Proto, std::unique_ptr<ExprAST> Body)
        : Proto(std::move(Proto)), Body(std::move(Body)) {}

    const PrototypeAST &getProto() const { return *Proto; }
    const ExprAST &getBody() const { return *Body; }

    virtual void accept(ExprASTVisitor &v) const override { v.visit(*this); }
};

class IfExprAST : public ExprAST
{
    std::unique_ptr<ExprAST> Cond, Then, Else;

public:
    IfExprAST(std::unique_ptr<ExprAST> Cond, std::unique_ptr<ExprAST> Then, std::unique_ptr<ExprAST> Else)
        : Cond(std::move(Cond)), Then(std::move(Then)), Else(std::move(Else)) {}

    const ExprAST &getCond() const { return *Cond; }
    const ExprAST &getThen() const { return *Then; }
    const ExprAST &getElse() const { return *Else; }

    virtual void accept(ExprASTVisitor &v) const override { v.visit(*this); }
};

class ForExprAST : public ExprAST
{
    std::string VarName;
    std::unique_ptr<ExprAST> Start, End, Step, Body;

public:
    ForExprAST(const std::string &VarName, std::unique_ptr<ExprAST> Start, std::unique_ptr<ExprAST> End, std::unique_ptr<ExprAST> Step, std::unique_ptr<ExprAST> Body)
        : Start(std::move(Start)), End(std::move(End)), Step(std::move(Step)), Body(std::move(Body)) {}

    const std::string &getVarName() const { return VarName; }
    const ExprAST &getStart() const { return *Start; }
    const ExprAST &getEnd() const { return *End; }
    bool isStepValid() const { return Step.get(); }
    const ExprAST &getStep() const { return *Step; }
    const ExprAST &getBody() const { return *Body; }

    virtual void accept(ExprASTVisitor &v) const override { v.visit(*this); }
};
