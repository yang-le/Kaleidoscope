#pragma once

#include "llvm/ADT/StringRef.h"
#include "llvm/ExecutionEngine/Orc/CompileOnDemandLayer.h"
#include "llvm/ExecutionEngine/Orc/CompileUtils.h"
#include "llvm/ExecutionEngine/Orc/Core.h"
#include "llvm/ExecutionEngine/Orc/ExecutionUtils.h"
#include "llvm/ExecutionEngine/Orc/IRCompileLayer.h"
#include "llvm/ExecutionEngine/Orc/IRTransformLayer.h"
#include "llvm/ExecutionEngine/Orc/JITTargetMachineBuilder.h"
#include "llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h"
#include "llvm/ExecutionEngine/Orc/TPCIndirectionUtils.h"
#include "llvm/ExecutionEngine/Orc/TargetProcessControl.h"
#include "llvm/ExecutionEngine/SectionMemoryManager.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include <memory>

namespace llvm::orc
{
    class KaleidoscopeJIT
    {
    private:
        std::unique_ptr<ExecutionSession> ES;
        std::unique_ptr<TPCIndirectionUtils> TPCIU;

        DataLayout DL;
        MangleAndInterner Mangle;

        RTDyldObjectLinkingLayer ObjectLayer;
        IRCompileLayer CompileLayer;
        IRTransformLayer OptimizeLayer;
        CompileOnDemandLayer CODLayer;

        JITDylib &MainJD;

        static void handleLazyCallThroughError()
        {
            errs() << "LazyCallThrough error: Counld not find function body";
            exit(1);
        }

    public:
        KaleidoscopeJIT(std::unique_ptr<ExecutionSession> ES, std::unique_ptr<TPCIndirectionUtils> TPCIU, JITTargetMachineBuilder JTMB, DataLayout DL)
            : ES(std::move(ES)), TPCIU(std::move(TPCIU)), DL(std::move(DL)), Mangle(*this->ES, this->DL),
              ObjectLayer(*this->ES, []()
                          { return std::make_unique<SectionMemoryManager>(); }),
              CompileLayer(*this->ES, ObjectLayer, std::make_unique<ConcurrentIRCompiler>(std::move(JTMB))),
              OptimizeLayer(*this->ES, CompileLayer, optimizeModule),
              CODLayer(*this->ES, OptimizeLayer, this->TPCIU->getLazyCallThroughManager(), [this]
                       { return this->TPCIU->createIndirectStubsManager(); }),
              MainJD(this->ES->createBareJITDylib("<main>"))
        {
            MainJD.addGenerator(cantFail(DynamicLibrarySearchGenerator::GetForCurrentProcess(DL.getGlobalPrefix())));
        }

        ~KaleidoscopeJIT()
        {
            if (auto Err = ES->endSession())
                ES->reportError(std::move(Err));

            if (auto Err = TPCIU->cleanup())
                ES->reportError(std::move(Err));
        }

        static Expected<std::unique_ptr<KaleidoscopeJIT>> Create()
        {
            auto SSP = std::make_shared<SymbolStringPool>();
            auto TPC = SelfTargetProcessControl::Create(SSP);
            if (!TPC)
                return TPC.takeError();

            auto ES = std::make_unique<ExecutionSession>(std::move(SSP));

            auto TPCIU = TPCIndirectionUtils::Create(**TPC);
            if (!TPCIU)
                return TPCIU.takeError();

            (*TPCIU)->createLazyCallThroughManager(*ES, pointerToJITTargetAddress(&handleLazyCallThroughError));

            if (auto Err = setUpInProcessLCTMReentryViaTPCIU(**TPCIU))
                return std::move(Err);

            JITTargetMachineBuilder JTMB((*TPC)->getTargetTriple());

            auto DL = JTMB.getDefaultDataLayoutForTarget();
            if (!DL)
                return DL.takeError();

            return std::make_unique<KaleidoscopeJIT>(std::move(ES), std::move(*TPCIU), std::move(JTMB), std::move(*DL));
        }

        const DataLayout &getDataLayout() const { return DL; }

        JITDylib &getMainJITDylib() { return MainJD; }

        Error addModule(ThreadSafeModule TSM, ResourceTrackerSP RT = nullptr)
        {
            if (!RT)
                RT = MainJD.getDefaultResourceTracker();

            return OptimizeLayer.add(RT, std::move(TSM));
        }

        Expected<JITEvaluatedSymbol> lookup(StringRef Name)
        {
            return ES->lookup({&MainJD}, Mangle(Name));
        }

    private:
        static Expected<ThreadSafeModule> optimizeModule(ThreadSafeModule TSM, const MaterializationResponsibility &R)
        {
            TSM.withModuleDo(
                [](Module &M)
                {
                    auto FPM = std::make_unique<legacy::FunctionPassManager>(&M);

                    FPM->add(createInstructionCombiningPass());
                    FPM->add(createReassociatePass());
                    FPM->add(createGVNPass());
                    FPM->add(createCFGSimplificationPass());

                    FPM->doInitialization();

                    for (auto &F : M)
                        FPM->run(F);
                });

            return std::move(TSM);
        }
    };
}
