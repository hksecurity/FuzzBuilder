#include "irireader.h"

#include "llvm/IR/Instructions.h"

IRIReader::IRIReader(Instruction* i) {
    this->i = i;
}

Function* IRIReader::get_callee() const {
    if(isa<CallInst>(this->i)) {
        CallInst* ci = dyn_cast<CallInst>(this->i);
        if(ci == nullptr) {
            return nullptr;
        }

        Function* callee = ci->getCalledFunction();
        if(callee == nullptr) {
            return nullptr;
        }

        return callee;
    } 
    else if(isa<InvokeInst>(this->i)) {
        InvokeInst* ii = dyn_cast<InvokeInst>(this->i);
        if(ii == nullptr) {
            return nullptr;
        }

        Function* callee = ii->getCalledFunction();
        if(callee == nullptr) {
            return nullptr;
        }

        return callee;
    }

    return nullptr;
}

