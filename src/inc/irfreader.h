#ifndef __IRFREADER_H__
#define __IRFREADER_H__

#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"

#include <set>

using namespace llvm;
using namespace std;

class IRFReader {
    private:
        Function* f;

    public:
        IRFReader(Function* f);
        set<Instruction*> get_asserts() const;

};

#endif
