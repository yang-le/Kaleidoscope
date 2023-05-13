#pragma once

#include "CodeGenVisitor.hh"
#include "llvm/Target/TargetMachine.h"

class NativeCodeGenVisitor : public CodeGenVisitor {
    llvm::TargetMachine *TargetMachine;
    void InitializeNative();

public:
    NativeCodeGenVisitor() : CodeGenVisitor()
    {
        InitializeNative();
    }

    ~NativeCodeGenVisitor() {
        // it seems that there are memory leaks in TargetMachine
        // delete here to avoid the leaks
        delete TargetMachine;
    }

    void emit(const std::string& filename) const;
};
