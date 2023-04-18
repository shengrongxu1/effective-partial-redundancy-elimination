#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Analysis/BlockFrequencyInfo.h"
#include "llvm/Analysis/BranchProbabilityInfo.h"
#include "llvm/Support/Format.h"
#include "llvm/IR/InstIterator.h"
#include <llvm/IR/Function.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Transforms/Scalar/Reassociate.h>
#include <llvm/Transforms/Utils.h>
#include <llvm/Analysis/LoopAnalysisManager.h>
#include <llvm/Passes/PassBuilder.h>
#include "llvm/IR/Dominators.h"
#include "llvm/Transforms/Utils.h"
#include "llvm/Transforms/Utils/PromoteMemToReg.h"
#include <unordered_map>
// LCM
//  Optimal Computation Points
// 1. Safe-Earliest Transformation: insert h = t at every entry of node n satisfying DSafe & Earliest and replace each t by h
// (1) down-safe (DSafe): computation of t at n does not introduce a new value on a terminating path starting in n.
// (2) Earliest: path from start to end, no node m prior to n is safe and delivers the same value as n when computing t.
// 
//  Suppress Unnecessary Code Motion
// 2. Latest Transformation: insert h = t at every entry of node n satisfying Delay & Latest and replace each t by h
// (3) Delay (from DSafe and Earliest) --> Latest: values
// 
//  LCM Transformatio
// 3. Optimal Transformation
// (4) Isolated & Latest --> OCP; RO --> insert h = t at every entry of node n in OCP and replace each t by h in RO.

using namespace llvm;
using namespace std;
namespace{
	struct PRE: public FunctionPass{
        static char ID;
        //FOR legacy LLVM
		PRE() : FunctionPass(ID) {
        }
        virtual bool runOnFunction(Function &F) override{
            // DominatorTree DT = DominatorTree(F);
            // PromoteMemToReg(F.getParent()->getDataLayout()).runOnFunction(F);
           
            return false;

        }
        //For LLVM 4.0
        // PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM) {
            
        //     FunctionPassManager function_pass_manager;


        //     function_pass_manager.addPass(ReassociatePass());
        //     function_pass_manager.run(F, FAM);

        //     return PreservedAnalyses::none();
        // }
        std::unordered_map<llvm::Value*,int> rankMap;
        void assignRank(Function &F) {
            for (auto &BB : F) {
                for (auto &I : BB){
                    if (llvm::StoreInst *SI = &llvm::dyn_cast<llvm::StoreInst>(I)){
                        llvm::Value *op_SI = SI->getOperand(1);
                        if (llvm::Constant *CI = &llvm::dyn_cast<llvm::Constant>(*op_SI)){
                            rankMap[SI->getOperand(0)] = 0;
                        }
                    
                    }
                    else if (llvm::BinaryOpIntrinsic *BI = &llvm::dyn_cast<llvm::BinaryOpIntrinsic>(I)){
                        llvm::Value *op_SI_l = BI->getOperand(1);
                        llvm::Value *op_SI_r = BI->getOperand(2);
                        rankMap[BI->getOperand(0)] = max(rankMap[op_SI_l],rankMap[op_SI_r]);

                    }

                }
            }
        }

    };
}
char PRE::ID = 0;
static RegisterPass<PRE> X("PRE", "PRE pass",
    false /* Only looks at CFG */,
    false /* Analysis Pass */);