#include <string>
#include <vector>

#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"

using namespace llvm;

class ExprAST {
public:
    virtual ~ExprAST() = default;

    virtual Value* codegen() = 0;
};

class ExprListAST {
    std::vector<ExprAST*> List;

public:
    ExprListAST() = default;

    ExprListAST(ExprAST* Expr) {
        List.push_back(Expr);
    }

    ~ExprListAST() {
        for (auto& e : List)
            delete e;
    }
    
    void push_back(ExprAST* Expr) {
        List.push_back(Expr);
    }

    size_t size() const noexcept {
        return List.size();
    }

    ExprAST*& operator[](size_t pos) {
        return List[pos];
    }

    void codegen() {
        for (auto& expr : List)
            expr->codegen();
    }
};

class NumberExprAST : public ExprAST {
    double Val;

public:
    NumberExprAST(double Val) : Val(Val) {}

    Value* codegen() override;
};

class VariableExprAST : public ExprAST {
    std::string Name;

public:
    VariableExprAST(const std::string& Name) : Name(Name) {}

    Value* codegen() override;
};

class VariableListAST {
    std::vector<std::string> List;

public:
    VariableListAST() = default;

    VariableListAST(const std::string& Var) {
        List.push_back(std::move(Var));
    }
    
    void push_back(const std::string& Var) {
        List.push_back(std::move(Var));
    }

    size_t size() const noexcept {
        return List.size();
    }

    std::string& operator[](size_t pos) {
        return List[pos];
    }
};

class BinaryExprAST : public ExprAST {
    char Op;
    ExprAST *LHS, *RHS;

public:
    BinaryExprAST(char Op, ExprAST* LHS, ExprAST* RHS)
        : Op(Op), LHS(LHS), RHS(RHS) {}

    Value* codegen() override;
};

class CallExprAST : public ExprAST {
    std::string Callee;
    ExprListAST* Args;

public:
    CallExprAST(const std::string& Callee, ExprListAST* Args)
        : Callee(Callee), Args(Args) {}

    Value* codegen() override;
};

class PrototypeAST : public ExprAST {
    std::string Name;
    VariableListAST* Args;

public:
    PrototypeAST(const std::string& Name, VariableListAST* Args)
        : Name(Name), Args(Args) {}

    const std::string& getName() const { return Name; }

    Value* codegen() override;
};

class FunctionAST : public ExprAST {
    PrototypeAST* Proto;
    ExprAST* Body;

public:
    FunctionAST(PrototypeAST* Proto, ExprAST* Body)
        : Proto(Proto), Body(Body) {}

    Value* codegen() override;
};

void codedump();
