#ifndef __IRIREADER_H__
#define __IRIREADER_H__

#include "llvm/IR/Instruction.h"
#include "llvm/IR/Function.h"

using namespace llvm;

class IRIReader {
    private:
        Instruction* i;

    public:
        IRIReader(Instruction* i);
        Function* get_callee() const;
};

#endif
