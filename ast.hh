#pragma once

#include <cassert>
#include <string>
#include <vector>
#include <memory>
#include <ostream>
#include "location.hh"

class ExprAST;
class NumberExprAST;
class VariableExprAST;
class BinaryExprAST;
class CallExprAST;
class PrototypeAST;
class FunctionAST;
class IfExprAST;
class ForExprAST;
class AssignExprAST;
class VarExprAST;

class ExprASTVisitor
{
public:
    virtual ~ExprASTVisitor() = default;

    virtual void visit(const ExprAST &e) = 0;
    virtual void visit(const NumberExprAST &n) = 0;
    virtual void visit(const VariableExprAST &v) = 0;
    virtual void visit(const BinaryExprAST &b) = 0;
    virtual void visit(const CallExprAST &c) = 0;
    virtual void visit(const PrototypeAST &p) = 0;
    virtual void visit(const FunctionAST &f) = 0;
    virtual void visit(const IfExprAST &i) = 0;
    virtual void visit(const ForExprAST &f) = 0;
    virtual void visit(const AssignExprAST &a) = 0;
    virtual void visit(const VarExprAST &a) = 0;
};

class ExprAST
{
    yy::location Loc;

public:
    ExprAST(yy::location Loc)
        : Loc(Loc) {}

    virtual ~ExprAST() = default;

    virtual void accept(ExprASTVisitor &v) const = 0;

    virtual std::ostream &dump(std::ostream &out, int ind) const;

    unsigned getLine() const { return Loc.begin.line; }
    unsigned getCol() const { return Loc.begin.column; }
};

class NumberExprAST : public ExprAST
{
    double Val;

public:
    NumberExprAST(yy::location Loc, double Val)
        : ExprAST(Loc), Val(Val) {}

    double getVal() const { return Val; }

    virtual void accept(ExprASTVisitor &v) const override { v.visit(*this); }

    virtual std::ostream &dump(std::ostream &out, int ind) const override;
};

class VariableExprAST : public ExprAST
{
    std::string Name;

public:
    VariableExprAST(yy::location Loc, const std::string &Name)
        : ExprAST(Loc), Name(Name) {}

    const std::string &getName() const { return Name; }

    virtual void accept(ExprASTVisitor &v) const override { v.visit(*this); }

    virtual std::ostream &dump(std::ostream &out, int ind) const override;
};

class BinaryExprAST : public ExprAST
{
    char Op;
    std::unique_ptr<ExprAST> LHS, RHS;

public:
    BinaryExprAST(yy::location Loc, char Op, std::unique_ptr<ExprAST> LHS, std::unique_ptr<ExprAST> RHS)
        : ExprAST(Loc), Op(Op), LHS(std::move(LHS)), RHS(std::move(RHS)) {}

    char getOp() const { return Op; }
    const ExprAST &getLHS() const { return *LHS; }
    const ExprAST &getRHS() const { return *RHS; }

    virtual void accept(ExprASTVisitor &v) const override { v.visit(*this); }

    virtual std::ostream &dump(std::ostream &out, int ind) const override;
};

class CallExprAST : public ExprAST
{
    std::string Callee;
    std::vector<std::unique_ptr<ExprAST>> Args;

public:
    CallExprAST(yy::location Loc, const std::string &Callee, std::vector<std::unique_ptr<ExprAST>> Args)
        : ExprAST(Loc), Callee(Callee), Args(std::move(Args)) {}

    const std::string &getCallee() const { return Callee; }
    const ExprAST &getArgs(std::size_t i) const { return *Args[i]; }
    size_t getArgsSize() const { return Args.size(); }

    virtual void accept(ExprASTVisitor &v) const override { v.visit(*this); }

    virtual std::ostream &dump(std::ostream &out, int ind) const override;
};

class PrototypeAST : public ExprAST
{
    std::string Name;
    std::vector<std::string> Args;

public:
    PrototypeAST(yy::location Loc, const std::string &Name, std::vector<std::string> Args)
        : ExprAST(Loc), Name(Name), Args(std::move(Args)) {}

    const std::string &getName() const { return Name; }
    const std::string &getArgs(std::size_t i) const { return Args[i]; }
    size_t getArgsSize() const { return Args.size(); }

    virtual void accept(ExprASTVisitor &v) const override { v.visit(*this); }

    virtual std::ostream &dump(std::ostream &out, int ind) const override;
};

class FunctionAST : public ExprAST
{
    std::unique_ptr<PrototypeAST> Proto;
    std::unique_ptr<ExprAST> Body;

public:
    FunctionAST(yy::location Loc, std::unique_ptr<PrototypeAST> Proto, std::unique_ptr<ExprAST> Body)
        : ExprAST(Loc), Proto(std::move(Proto)), Body(std::move(Body)) {}

    const PrototypeAST &getProto() const { return *Proto; }
    const ExprAST &getBody() const { return *Body; }

    virtual void accept(ExprASTVisitor &v) const override { v.visit(*this); }

    virtual std::ostream &dump(std::ostream &out, int ind) const override;
};

class IfExprAST : public ExprAST
{
    std::unique_ptr<ExprAST> Cond, Then, Else;

public:
    IfExprAST(yy::location Loc, std::unique_ptr<ExprAST> Cond, std::unique_ptr<ExprAST> Then, std::unique_ptr<ExprAST> Else)
        : ExprAST(Loc), Cond(std::move(Cond)), Then(std::move(Then)), Else(std::move(Else)) {}

    const ExprAST &getCond() const { return *Cond; }
    const ExprAST &getThen() const { return *Then; }
    const ExprAST &getElse() const { return *Else; }

    virtual void accept(ExprASTVisitor &v) const override { v.visit(*this); }

    virtual std::ostream &dump(std::ostream &out, int ind) const override;
};

class ForExprAST : public ExprAST
{
    std::string VarName;
    std::unique_ptr<ExprAST> Start, End, Step, Body;

public:
    ForExprAST(yy::location Loc, const std::string &VarName, std::unique_ptr<ExprAST> Start, std::unique_ptr<ExprAST> End, std::unique_ptr<ExprAST> Step, std::unique_ptr<ExprAST> Body)
        : ExprAST(Loc), VarName(VarName), Start(std::move(Start)), End(std::move(End)), Step(std::move(Step)), Body(std::move(Body)) {}

    const std::string &getVarName() const { return VarName; }
    const ExprAST &getStart() const { return *Start; }
    const ExprAST &getEnd() const { return *End; }
    bool isStepValid() const { return !!Step; }
    const ExprAST &getStep() const { return *Step; }
    const ExprAST &getBody() const { return *Body; }

    virtual void accept(ExprASTVisitor &v) const override { v.visit(*this); }

    virtual std::ostream &dump(std::ostream &out, int ind) const override;
};

class AssignExprAST : public ExprAST
{
    std::string VarName;
    std::unique_ptr<ExprAST> RHS;

public:
    AssignExprAST(yy::location Loc, const std::string &VarName, std::unique_ptr<ExprAST> RHS)
        : ExprAST(Loc), VarName(VarName), RHS(std::move(RHS)) {}

    const std::string &getVarName() const { return VarName; }
    const ExprAST &getRHS() const { return *RHS; }

    virtual void accept(ExprASTVisitor &v) const override { v.visit(*this); }

    virtual std::ostream &dump(std::ostream &out, int ind) const override;
};

class VarExprAST : public ExprAST
{
    std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> Vars;
    std::unique_ptr<ExprAST> Body;

public:
    VarExprAST(yy::location Loc, std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> Vars, std::unique_ptr<ExprAST> Body)
        : ExprAST(Loc), Vars(std::move(Vars)), Body(std::move(Body)) {}

    const std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> &getVars() const { return Vars; }
    const ExprAST &getBody() const { return *Body; }

    virtual void accept(ExprASTVisitor &v) const override { v.visit(*this); }

    virtual std::ostream &dump(std::ostream &out, int ind) const override;
};
