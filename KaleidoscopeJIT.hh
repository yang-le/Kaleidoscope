#pragma once

#include "llvm/ADT/StringRef.h"
#include "llvm/ExecutionEngine/Orc/CompileOnDemandLayer.h"
#include "llvm/ExecutionEngine/Orc/CompileUtils.h"
#include "llvm/ExecutionEngine/Orc/Core.h"
#include "llvm/ExecutionEngine/Orc/EPCIndirectionUtils.h"
#include "llvm/ExecutionEngine/Orc/ExecutionUtils.h"
#include "llvm/ExecutionEngine/Orc/ExecutorProcessControl.h"
#include "llvm/ExecutionEngine/Orc/IRCompileLayer.h"
#include "llvm/ExecutionEngine/Orc/IRTransformLayer.h"
#include "llvm/ExecutionEngine/Orc/JITTargetMachineBuilder.h"
#include "llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h"
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
        std::unique_ptr<EPCIndirectionUtils> EPCIU;

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
        KaleidoscopeJIT(std::unique_ptr<ExecutionSession> ES, std::unique_ptr<EPCIndirectionUtils> EPCIU, JITTargetMachineBuilder JTMB, DataLayout DL)
            : ES(std::move(ES)), EPCIU(std::move(EPCIU)), DL(std::move(DL)), Mangle(*this->ES, this->DL),
              ObjectLayer(*this->ES, []()
                          { return std::make_unique<SectionMemoryManager>(); }),
              CompileLayer(*this->ES, ObjectLayer, std::make_unique<ConcurrentIRCompiler>(std::move(JTMB))),
              OptimizeLayer(*this->ES, CompileLayer, optimizeModule),
              CODLayer(*this->ES, OptimizeLayer, this->EPCIU->getLazyCallThroughManager(), [this]
                       { return this->EPCIU->createIndirectStubsManager(); }),
              MainJD(this->ES->createBareJITDylib("<main>"))
        {
            MainJD.addGenerator(cantFail(DynamicLibrarySearchGenerator::GetForCurrentProcess(DL.getGlobalPrefix())));
        }

        ~KaleidoscopeJIT()
        {
            if (auto Err = ES->endSession())
                ES->reportError(std::move(Err));

            if (auto Err = EPCIU->cleanup())
                ES->reportError(std::move(Err));
        }

        static Expected<std::unique_ptr<KaleidoscopeJIT>> Create()
        {
            auto EPC = SelfExecutorProcessControl::Create();
            if (!EPC)
                return EPC.takeError();

            auto ES = std::make_unique<ExecutionSession>(std::move(*EPC));

            auto EPCIU = EPCIndirectionUtils::Create(ES->getExecutorProcessControl());
            if (!EPCIU)
                return EPCIU.takeError();

            (*EPCIU)->createLazyCallThroughManager(*ES, ExecutorAddr::fromPtr(&handleLazyCallThroughError));

            if (auto Err = setUpInProcessLCTMReentryViaEPCIU(**EPCIU))
                return std::move(Err);

            JITTargetMachineBuilder JTMB(ES->getExecutorProcessControl().getTargetTriple());

            auto DL = JTMB.getDefaultDataLayoutForTarget();
            if (!DL)
                return DL.takeError();

            return std::make_unique<KaleidoscopeJIT>(std::move(ES), std::move(*EPCIU), std::move(JTMB), std::move(*DL));
        }

        const DataLayout &getDataLayout() const { return DL; }

        JITDylib &getMainJITDylib() { return MainJD; }

        Error addModule(ThreadSafeModule TSM, ResourceTrackerSP RT = nullptr)
        {
            if (!RT)
                RT = MainJD.getDefaultResourceTracker();

            return CODLayer.add(RT, std::move(TSM));
        }

        Expected<ExecutorSymbolDef> lookup(StringRef Name)
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
