#include "llvm/Pass.h"
#include "llvm/IR/CFG.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Analysis/BlockFrequencyInfo.h"
#include "llvm/Analysis/BranchProbabilityInfo.h"
#include "llvm/Support/Format.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/InstIterator.h"
#include <llvm/IR/Function.h>
#include <llvm/IR/PassManager.h>
#include "llvm/IR/IRBuilder.h"
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
            // -------------------------------------------------------------------------
            // forwardProp testing
            forwardProp(&F);
            // print BBs
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
        // Rank Computing Done here
        //
        // Forward Propagation
        //  (1) Remove each Phi node x = Phi(y, z) by inserting the copies x = y and x = z
        //  (2) Trace from each copy back along the SSA graph (new blocks required) to construct expression trees
        //  (3) Check the uses and push expressions
        bool forwardProp(Function *F) {
            // Create a map to store the uses of Phi nodes
            BasicBlock *entryBB = &F->getEntryBlock();
            std::map<int, std::vector<PHINode *>> PhiNums;
            std::map<int, std::vector<BasicBlock *>> PhiSuccBBs;
            std::map<int, std::vector<BasicBlock *>> PhiAlongPathPredBBs;
            std::map<int, std::vector<BasicBlock *>> PhiOtherPathPredBBs;
            std::map<PHINode *, std::vector<Value *>> PhiUsesMap; // values of phi nodes?
            std::map<int, std::vector<BasicBlock *>> InsertedBBs;
            // Position of Phi nodes
            int numOfBB = 0;
            // Assuming we have all the Phi nodes in the pruned SSA form
            // -------------------------------------------------------------------------------
            // Get information
            for (BasicBlock &BB : *F) {
                numOfBB++;
                errs() << "number of BB: " << numOfBB << "\n";
                // Traverse the instructions in the basic block
                for (Instruction &I : BB) {
                    // check if the instruction is a phi node, store positions and all uses of phi nodes
                    if (auto *phi = dyn_cast<PHINode>(&I)) {
                        // store the Phi node at the number of succBB 
                        PhiNums[numOfBB].push_back(phi);
                        // store information of Preds & Succ for Splitedge
                        // check entry & exit nodes
                        if (std::find(PhiSuccBBs[numOfBB].begin(), PhiSuccBBs[numOfBB].end(), &BB) == PhiSuccBBs[numOfBB].end()) {
                            PhiSuccBBs[numOfBB].push_back(&BB);
                            // Get pred BBs on two paths
                            for (BasicBlock *Pred : predecessors(&BB)) {
                                if (PhiAlongPathPredBBs[numOfBB].empty()) {
                                    PhiAlongPathPredBBs[numOfBB].push_back(Pred);
                                }
                                else { 
                                    PhiOtherPathPredBBs[numOfBB].push_back(Pred);
                                }
                                errs() << Pred->getName() << "\n";
                            }
                            // testing
                            // for 
                            //     errs() << PhiOtherPathPredBBs[numOfBB] << "\n";
                            // }
                            // //
                        }
                        // get the uses of Phi nodes
                        for (auto op = phi->op_begin(); op != phi->op_end(); ++op) {
                            Value *U = op->get();
                            PhiUsesMap[phi].push_back(U);
                        }
                    }
                }
            }
            // -------------------------------------------------------------------------------
            // testing cases //
            /**/
            for (auto it = PhiNums.begin(); it != PhiNums.end(); ++it) {
                errs()<< "\n" << "Positions of BB: " << it->first << "\n" << "\n";
            }
            // -------------------------------------------------------------------------------
            // Create new BBs, for one phi node, insert two BBs from two Preds to Succ
            for (auto iv = PhiNums.begin(); iv != PhiNums.end(); ++iv) {
                // splitEdge at numOfBB (iv->first)
                // front
                InsertedBBs[iv->first].push_back(SplitEdge(PhiAlongPathPredBBs[iv->first].front(), PhiSuccBBs[iv->first].front()));
                // back
                InsertedBBs[iv->first].push_back(SplitEdge(PhiOtherPathPredBBs[iv->first].front(), PhiSuccBBs[iv->first].front()));
            }
            std::vector<Instruction *> alloc;
            std::vector<Instruction *> phiNodesVec;
            //-------------------------------------------------------------------------------
            for (map<int, std::vector<llvm::PHINode *>>::iterator iv = PhiNums.begin(); iv != PhiNums.end(); ++iv) { // number of backedge
                for (std::vector<llvm::PHINode *>::iterator it = iv->second.begin(); it != iv->second.end(); ++it) { // *PhiNode
                    Instruction *valPhi = *it;
                    Instruction *alloi = new AllocaInst(valPhi->getType(), 0, valPhi->getName(), entryBB->getTerminator());
                    alloc.push_back(alloi);
                    phiNodesVec.push_back(valPhi);
                }
            }
            //
            int cnt = 0;
            // Add new copies in the new BBs
            for (map<int, std::vector<llvm::PHINode *>>::iterator iv = PhiNums.begin(); iv != PhiNums.end(); ++iv) { // number of backede
                for (std::vector<llvm::PHINode *>::iterator it = iv->second.begin(); it != iv->second.end(); ++it) { // *PhiNode
                    // insert front value in PhiAlongPathPredBBs
                    Value *val = PhiUsesMap[*it].front();
                    Value *val2 = PhiUsesMap[*it].back();
                    Instruction *valPhi = *it;
                    // Create an alloca instruction for phi (assuming val is an integer).
                    //Instruction *alloi = new AllocaInst(val->getType(), 0, valPhi->getName(), InsertedBBs[iv->first].back()->getTerminator());
                    // Create a store instruction to store the value of val into
                    Instruction *str1 = new StoreInst(val2, alloc[cnt], InsertedBBs[iv->first].front()->getTerminator());
                    Instruction *str2 = new StoreInst(val, alloc[cnt], InsertedBBs[iv->first].back()->getTerminator());
                    cnt++;
                    // insert back value in PhiOtherPathPredBBs
                    // Create an alloca instruction for phi (assuming val is an integer).
                    // Instruction *alloi2 = new AllocaInst(val2->getType(), 0, "", InsertedBBs[iv->first].back()->getTerminator());
                    // // Create a store instruction to store the value of val into 
                    //Instruction *str2 = new StoreInst(val2, valPhi, InsertedBBs[iv->first].back()->getTerminator());
                    // --------------------------------------------------------------------------------------
                    //for (auto val = PhiUsesMap[*it].begin(); val != PhiUsesMap[*it].end(); ++val) { // Values
                        // Check uses in the Preds --> add in BB
                        // iterate instructions in AlongPathPred BB
                        // for (Instruction &I : *PhiAlongPathPredBBs[iv->first].front()) {
                        //     // iterate over users in instructions
                        //     for (Use &U : I.uses()) {
                        //         if (Instruction *useInst = dyn_cast<Instruction>(U)) {
                        //             // uses v.s. phi uses
                        //             User *userInst = U.getUser();
                        //             // errs() << "Define useInst" << "\n";
                        //             if (useInst == *val) {
                        //                 // errs() << "useInst == value" << "\n";
                        //                 // add x=y to the Pred BB
                        //                 // use IRBuilder at the new BB
                        //                 IRBuilder<> builder(InsertedBBs[iv->first].front()->getTerminator());
                        //                 // Create an alloca instruction for phi (assuming val is an integer).
                        //                 AllocaInst *it = builder.CreateAlloca((*val)->getType(), nullptr);
                        //                 // Create a store instruction to store the value of val into 
                        //                 builder.CreateStore(*val, it);
                        //                 //
                        //                 errs() << "Add instruction AlongPath;" << "\n";
                        //             }
                        //         }
                        //     }
                        // }
                        // iterate instructions in PhiOtherPathPredBBs
                        // for (Instruction &I : *PhiOtherPathPredBBs[iv->first].front()) {
                        //     // iterate over users in instructions
                        //     for (Use &U : I.uses()) {
                        //         if (Instruction *useInst = dyn_cast<Instruction>(U)) {
                        //             // uses v.s. phi uses
                        //             User *userInst = U.getUser();
                        //             // errs() << "Define useInst" << "\n";
                        //             if (useInst == *val) {
                        //                 // add x=y to the Pred BB
                        //                 // use IRBuilder
                        //                 IRBuilder<> Obuilder(InsertedBBs[iv->first].back()->getTerminator());
                        //                 // Create an alloca instruction for phi (assuming val is an integer).
                        //                 AllocaInst *it = Obuilder.CreateAlloca((*val)->getType(), nullptr);
                        //                 // Create a store instruction to store the value of val into 
                        //                 Obuilder.CreateStore(*val, it);
                        //                 //
                        //                 errs() << "Add instruction OtherPath;" << "\n";
                        //             }
                        //         }
                        //     }
                        // }
                    //}
                    // --------------------------------------------------------------------------------------
                }
            }
            // -------------------------------------------------------------------------------
            // change phi uses to store uses in every BB
            for (int count = 0; count < cnt; count++) {
                // current phi node
                Value *valPhi = phiNodesVec[count];
                // iterate over BBs
               for (Function::iterator BB = F->begin(), E = F->end(); BB != E; ++BB) {
                    for (BasicBlock::iterator i = BB->begin(), be = BB->end(); i != be; ++i){
                        Instruction* inst = &(*i);
                        //iterate over users of instructions
                        int opcnt = 0;
                        for (User::op_iterator op = inst->op_begin(), opEnd = inst->op_end(); op != opEnd; ++op) {
                        // Check if the operand matches the pointer
                            if (*op == valPhi) {
                                
                                LoadInst* loadInst = new LoadInst(valPhi->getType(),alloc[count], "myLoad", inst);

                                // Set the operand to the loaded value
                                inst->setOperand(opcnt, loadInst); 
                                
                                // Create a PtrToInt instruction to convert the pointer type to i32 type
                                // Type *i32Type = Type::getInt32Ty(F->getContext());
                                // Instruction *castInst = new PtrToIntInst(alloc[i], i32Type);
                                // replace
                                //alloc[i]
                                // I.setOperand(i, alloc[i]);
                                //(&I)->getParent()->getInstList().insertAfter((&I)->getIterator(), castInst);
                            }
                            opcnt++;
                        }
                        opcnt = 0;
                    }
                }
            }
            // -------------------------------------------------------------------------------
            //remove all the phi nodes
            for (map<int, std::vector<llvm::PHINode *>>::iterator iv = PhiNums.begin(); iv != PhiNums.end(); ++iv) { // number
                for (std::vector<llvm::PHINode *>::iterator it = iv->second.begin(); it != iv->second.end(); ++it) { // *Phis
                    (*it)->eraseFromParent();
                }
            }
            //-------------------------------------------------------------------------------
            return true;
        }

        //retrun ordered operands for each value
        //input: vactor of all phi uses
        //outPut: map of ordered usses operands
        //          key:use  val: rank ordered operands
        std::map<Value *,std::vector<std::pair<llvm::Value *, int>>> sortAllPHIOperands(std::vector<Value *> vals){
            std::map<Value *,std::vector<std::pair<llvm::Value *, int>>> phiMap;
            for(int i=0;i<vals.size();i++){
                std::map<llvm::Value *, int> temp;
                getOperads(vals[i],temp);
                phiMap[vals[i]]=sortMapByValue(temp);
            }
            return phiMap;
        }
        
        void getOperads(Value * val, std::map<llvm::Value *, int>valRankMap){
            if(llvm::cast<llvm::Instruction>(val)->getOpcode() == llvm::Instruction::Add){
                getOperads(llvm::cast<llvm::Instruction>(val)->getOperand(0),valRankMap);
                getOperads(llvm::cast<llvm::Instruction>(val)->getOperand(1),valRankMap);
            }else{
                valRankMap[val]=getRank(val);
                return;
            }
        }

        int getRank(llvm::Value * exp){
            if(rankMap.find(exp) != rankMap.end()){
                return rankMap[exp];
            }else{
                return -1;
            }
        }

        static bool compareByValue(const std::pair<llvm::Value *, int> &a, const std::pair<llvm::Value *, int> &b) {
            return a.second < b.second;
        }

        std::vector<std::pair<llvm::Value *, int>> sortMapByValue(const std::map<llvm::Value *, int> &m) {
            std::vector<std::pair<llvm::Value *, int>> sortedPairs(m.begin(), m.end());
            std::sort(sortedPairs.begin(), sortedPairs.end(), compareByValue);
            return sortedPairs;
        }

    };
}
char PRE::ID = 0;
static RegisterPass<PRE> X("PRE", "PRE pass",
                           false /* Only looks at CFG */,
                           false /* Analysis Pass */);