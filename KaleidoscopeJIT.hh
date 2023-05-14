#pragma once

#include "llvm/ADT/StringRef.h"
#include "llvm/ExecutionEngine/Orc/CompileUtils.h"
#include "llvm/ExecutionEngine/Orc/Core.h"
#include "llvm/ExecutionEngine/Orc/ExecutionUtils.h"
#include "llvm/ExecutionEngine/Orc/TargetProcessControl.h"
#include "llvm/ExecutionEngine/Orc/IRCompileLayer.h"
#include "llvm/ExecutionEngine/Orc/JITTargetMachineBuilder.h"
#include "llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h"
#include "llvm/ExecutionEngine/SectionMemoryManager.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/LLVMContext.h"
#include <memory>

namespace llvm::orc
{
    class KaleidoscopeJIT
    {
    private:
        std::unique_ptr<ExecutionSession> ES;

        DataLayout DL;
        MangleAndInterner Mangle;

        RTDyldObjectLinkingLayer ObjectLayer;
        IRCompileLayer CompileLayer;

        JITDylib &MainJD;

    public:
        KaleidoscopeJIT(std::unique_ptr<ExecutionSession> ES, JITTargetMachineBuilder JTMB, DataLayout DL)
            : ES(std::move(ES)), DL(std::move(DL)), Mangle(*this->ES, this->DL),
              ObjectLayer(*this->ES, []()
                          { return std::make_unique<SectionMemoryManager>(); }),
              CompileLayer(*this->ES, ObjectLayer, std::make_unique<ConcurrentIRCompiler>(std::move(JTMB))),
              MainJD(this->ES->createBareJITDylib("<main>"))
        {
            MainJD.addGenerator(cantFail(DynamicLibrarySearchGenerator::GetForCurrentProcess(DL.getGlobalPrefix())));
        }

        ~KaleidoscopeJIT()
        {
            if (auto Err = ES->endSession())
                ES->reportError(std::move(Err));
        }

        static Expected<std::unique_ptr<KaleidoscopeJIT>> Create()
        {
            auto SSP = std::make_shared<SymbolStringPool>();
            auto TPC = SelfTargetProcessControl::Create(SSP);
            if (!TPC)
                return TPC.takeError();

            auto ES = std::make_unique<ExecutionSession>(std::move(SSP));

            JITTargetMachineBuilder JTMB((*TPC)->getTargetTriple());

            auto DL = JTMB.getDefaultDataLayoutForTarget();
            if (!DL)
                return DL.takeError();

            return std::make_unique<KaleidoscopeJIT>(std::move(ES), std::move(JTMB), std::move(*DL));
        }

        const DataLayout &getDataLayout() const { return DL; }

        JITDylib &getMainJITDylib() { return MainJD; }

        Error addModule(ThreadSafeModule TSM, ResourceTrackerSP RT = nullptr)
        {
            if (!RT)
                RT = MainJD.getDefaultResourceTracker();
            return CompileLayer.add(RT, std::move(TSM));
        }

        Expected<JITEvaluatedSymbol> lookup(StringRef Name)
        {
            return ES->lookup({&MainJD}, Mangle(Name));
        }
    };
}