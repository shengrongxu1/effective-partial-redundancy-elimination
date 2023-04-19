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
#include "llvm/Transforms/Utils/SSAUpdater.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include <unordered_map>
#include <vector>
#include <algorithm>
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
namespace
{
    struct PRE : public FunctionPass
    {
        static char ID;
        // FOR legacy LLVM
        PRE() : FunctionPass(ID)
        {
        }
        std::unordered_map<llvm::Value *, int> rankMap;
        virtual bool runOnFunction(Function &F) override
        {
            // DominatorTree DT = DominatorTree(F);
            // PromoteMemToReg(F.getParent()->getDataLayout()).runOnFunction(F);
            assignRank(F);
            for (const auto &kv : rankMap)
            {
                llvm::Value *key = kv.first;
                int value = kv.second;

                // Assuming you have access to the LLVM context, you can use the following
                // code to print the key (llvm::Value*) in a human-readable format
                key->print(llvm::errs(), false); // Set the second parameter to true if you want to print the type as well
                llvm::errs() << " -> " << value << "\n";
            }

            for (auto &BB : F) {
                errs() << BB << "\n";
            }
            return false;
        }

        // Assume that 'F' is a pointer to the function for which you want to construct
        // the SSA form with phi nodes.

        // This function takes a pointer to a function and converts it to SSA form with phi nodes.

        // For LLVM 4.0
        //  PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM) {

        //     FunctionPassManager function_pass_manager;

        //     function_pass_manager.addPass(ReassociatePass());
        //     function_pass_manager.run(F, FAM);

        //     return PreservedAnalyses::none();
        // }
        std::vector<BasicBlock *> getFrequentPath(Function *F)
        {
            BranchProbabilityInfo &bpi = getAnalysis<BranchProbabilityInfoWrapperPass>().getBPI();

            // Find the entry block
            BasicBlock *entryBB = &F->getEntryBlock();
            std::vector<BasicBlock *> frequentPath;
            BasicBlock *BB = entryBB;
            while (true)
            {
                BasicBlock *next = nullptr;
                for (succ_iterator PI = succ_begin(BB); PI != succ_end(BB); ++PI)
                {
                    next = *PI;
                    BranchProbability currProInfo = bpi.getEdgeProbability(BB, next);
                    float currPro = (float)currProInfo.getNumerator() / (float)currProInfo.getDenominator();
                    if (currPro >= 0.8)
                    {
                        break;
                    }
                }
                frequentPath.push_back(next);
                if (next == entryBB)
                {
                    break;
                }
                BB = next;
            }
            return frequentPath;
        }

        void assignRank(Function &F)
        {
            int bb_num = 1;
            for (auto &BB : F)
            {

                for (auto &I : BB)
                {
                    // if (llvm::StoreInst *SI = &llvm::dyn_cast<llvm::StoreInst>(I)){
                    //     llvm::Value *op_SI = SI->getOperand(1);
                    //     if (llvm::Constant *CI = &llvm::dyn_cast<llvm::Constant>(*op_SI)){
                    //         rankMap[SI->getOperand(0)] = 0;
                    //     }

                    // }
                    // else if (llvm::PHINode *PHI = &llvm::dyn_cast<llvm::PHINode>(I)){
                    //     rankMap[PHI->getOperand(0)] = bb_num;
                    // }
                    // else if (llvm::BinaryOpIntrinsic *BI = &llvm::dyn_cast<llvm::BinaryOpIntrinsic>(I)){
                    //     llvm::Value *op_SI_l = BI->getOperand(1);
                    //     llvm::Value *op_SI_r = BI->getOperand(2);
                    //     rankMap[BI->getOperand(0)] = max(rankMap[op_SI_l],rankMap[op_SI_r]);

                    // }
                    if (llvm::isa<llvm::StoreInst>(I))
                    {
                        llvm::StoreInst *SI = llvm::cast<llvm::StoreInst>(&I);
                        llvm::Value *op_SI = SI->getOperand(0);
                        if (llvm::isa<llvm::Constant>(*op_SI))
                        {
                            llvm::Constant *CI = llvm::cast<llvm::Constant>(op_SI);
                            llvm::Value *Dst = SI; 
                            rankMap[Dst] = 0;
                        }
                    }
                    else if (llvm::isa<llvm::PHINode>(I))
                    {
                        llvm::PHINode *PHI = llvm::cast<llvm::PHINode>(&I);
                        llvm::Value *Dst = PHI;
                        rankMap[Dst] = bb_num;
                    }
                    else if (llvm::isa<llvm::BinaryOperator>(I))
                    {
                        llvm::BinaryOperator *BI = llvm::cast<llvm::BinaryOperator>(&I);
                        llvm::Value *op_SI_l = BI->getOperand(0);
                        llvm::Value *op_SI_r = BI->getOperand(1);
                        llvm::Value *Dst = BI;
                        rankMap[Dst] = std::max(rankMap[op_SI_l], rankMap[op_SI_r]);
                    }
                }
                bb_num += 1;
            }
        }

        //
        // Rank Computing
        //
        // Forward Propagation
        //  (1) Remove each Phi node x = Phi(y, z) by inserting the copies x = y and x = z
        //  (2) Trace from each copy back along the SSA graph (new blocks required) to construct expression trees
        //  (3) Check the uses and push expressions
        bool forwardProp(Function &F) {
            // Create a map to store the uses of Phi nodes
            std::map<int, std::vector<PHINode *>> PhiNums;
            std::map<int, std::vector<BasicBlock *>> PhiSuccBBs;
            std::map<int, std::vector<BasicBlock *>> PhiAlongPathPredBBs;
            std::map<int, std::vector<BasicBlock *>> PhiOtherPathPredBBs;
            std::map<PHINode *, std::vector<Value *>> PhiUsesMap; // values of phi nodes? Not sure.
            std::map<int, std::vector<BasicBlock *>> InsertedBBs;
            // Position of Phi nodes
            int numOfBB = 0;
            // Assuming we have all the Phi nodes in the pruned SSA form here
            // Traverse the function's basic blocks
            for (BasicBlock &BB : F) {
                numOfBB++;
                // Traverse the instructions in the basic block
                for (Instruction &I : BB) {
                    // check if the instruction is a phi node, store positions and all uses of phi nodes
                    if (auto *phi = dyn_cast<PHINode>(&I)) {
                        // store the Phi node at the number of succBB 
                        PhiNums[numOfBB].push_back(phi);
                        // store information of Preds & Succ for Splitedge
                        if (std::find(PhiSuccBBs[numOfBB].begin(), PhiSuccBBs[numOfBB].end(), &BB) == PhiSuccBBs[numOfBB].end()) {
                            PhiSuccBBs[numOfBB].push_back(&BB);
                            // Get pred BBs on two paths
                            for (BasicBlock *Pred : predecessors(&BB)) {
                                if (PhiAlongPathPredBBs[numOfBB].empty()) {PhiAlongPathPredBBs[numOfBB].push_back(Pred);}
                                else {PhiOtherPathPredBBs[numOfBB].push_back(Pred);}
                            }
                        }
                            // get the uses of Phi nodes
                        for (auto op = phi->op_begin(); op != phi->op_end(); ++op) {
                            Value *U = op->get();
                            PhiUsesMap[phi].push_back(U);
                        }
                    }
                }
            }
            //
            // Create new BBs
            for (auto iv = PhiNums.begin(); iv != PhiNums.end(); ++iv) {
                // iterating over Phi nodes (vectors)
                for (auto it = iv->second.begin(); it != iv->second.end(); ++it) {
                    // splitEdge at numOfBB (iv->first)
                    // insert two BBs from two Preds to Succ
                    InsertedBBs[iv->first].push_back(SplitEdge(PhiAlongPathPredBBs[iv->first].front(), PhiSuccBBs[iv->first].front()));
                    InsertedBBs[iv->first].push_back(SplitEdge(PhiOtherPathPredBBs[iv->first].front(), PhiSuccBBs[iv->first].front()));
                }
            }
            // Convert phi nodes in the new BBs
            // ......
            return true;
        }
    };
}
char PRE::ID = 0;
static RegisterPass<PRE> X("PRE", "PRE pass",
                           false /* Only looks at CFG */,
                           false /* Analysis Pass */);