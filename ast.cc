#include "ast.hh"

namespace
{
    std::ostream &indent(std::ostream &O, int size)
    {
        return O << std::string(size * 2, ' ');
    }
}

std::ostream &FunctionAST::dump(std::ostream &out, int ind) const
{
    ExprAST::dump(indent(out, ind) << "function", ind + 1);
    Proto->dump(indent(out, ind) << "Proto:", ind + 1);
    Body->dump(indent(out, ind) << "Body:", ind + 1);
    return out;
}

std::ostream &IfExprAST::dump(std::ostream &out, int ind) const
{
    ExprAST::dump(out << "if", ind + 1);
    Cond->dump(indent(out, ind) << "Cond:", ind + 1);
    Then->dump(indent(out, ind) << "Then:", ind + 1);
    Else->dump(indent(out, ind) << "Else:", ind + 1);
    return out;
}

std::ostream &ForExprAST::dump(std::ostream &out, int ind) const
{
    ExprAST::dump(out << "for", ind + 1);
    Start->dump(indent(out, ind) << "Start:", ind + 1);
    End->dump(indent(out, ind) << "End:", ind + 1);
    if (Step)
        Step->dump(indent(out, ind) << "Step:", ind + 1);
    Body->dump(indent(out, ind) << "Body:", ind + 1);
    return out;
}

std::ostream &VarExprAST::dump(std::ostream &out, int ind) const
{
    ExprAST::dump(out << "var", ind + 1);
    for (const auto &[Name, Expr] : Vars)
        Expr ? Expr->dump(indent(out, ind) << Name << ':', ind + 1) : indent(out, ind) << Name << '\n';
    Body->dump(indent(out, ind) << "Body:", ind + 1);
    return out;
}

std::ostream &PrototypeAST::dump(std::ostream &out, int ind) const
{
    ExprAST::dump(out << "prototype " << Name, ind + 1);
    for (const auto &Arg : Args)
        ExprAST::dump(indent(out, ind) << Arg, ind + 1);
    return out;
}

std::ostream &CallExprAST::dump(std::ostream &out, int ind) const
{
    ExprAST::dump(out << "call " << Callee, ind + 1);
    for (const auto &Arg : Args)
        Arg->dump(indent(out, ind), ind + 1);
    return out;
}

std::ostream &BinaryExprAST::dump(std::ostream &out, int ind) const
{
    ExprAST::dump(out << "binary" << Op, ind + 1);
    LHS->dump(indent(out, ind) << "LHS:", ind + 1);
    RHS->dump(indent(out, ind) << "RHS:", ind + 1);
    return out;
}

std::ostream &VariableExprAST::dump(std::ostream &out, int ind) const
{
    return ExprAST::dump(out << Name, ind + 1);
}

std::ostream &NumberExprAST::dump(std::ostream &out, int ind) const
{
    return ExprAST::dump(out << Val, ind + 1);
}

std::ostream &ExprAST::dump(std::ostream &out, int ind) const
{
    return out << ':' << Loc << '\n';
}

std::ostream &AssignExprAST::dump(std::ostream &out, int ind) const
{
    ExprAST::dump(out << "assign " << VarName, ind + 1);
    RHS->dump(indent(out, ind) << "RHS:", ind + 1);
    return out;
}
