#include "irfreader.h"

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instructions.h"

IRFReader::IRFReader(Function* f) {
    this->f = f;
}

set<Instruction*> IRFReader::get_asserts() const {
    set<Instruction*> ret;

    for(BasicBlock& b : *(this->f)) {
        for(Instruction& i : b) {
            if(!isa<CallInst>(&i)) {
                continue;
            }

            CallInst* ci = dyn_cast<CallInst>(&i);
            Function* callee = ci->getCalledFunction();
            if(callee == nullptr) {
                continue;
            }

            string name = string(callee->getName());
            if(name != "abort" && name != "__assert_fail") {
                continue;
            }

            ret.insert(&i); 
        }
    }

    return ret;
}
