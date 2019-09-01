#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"

#include <set>

using namespace llvm;
using namespace std;

class IRReader {
    private:
        Module* m;
        set<Function*> get_gtest_functions() const;
        set<Instruction*> get_calls(Function& f) const;
        set<Function*> get_callees(Function& f) const;
        set<Function*> get_test_functions() const;
        set<Function*> get_target_functions() const;

        bool is_target(Function& f) const;
        bool is_target(Instruction& i) const;
        bool is_test(Function& f) const;

    public:
        IRReader(Module* m);
        set<Function*> get_functions_to_fuzz() const;
        set<Function*> get_functions_to_remove() const;
        set<Function*> get_functions_to_collect() const;
        set<Instruction*> get_target_instructions(string n) const;

//        Function* get_callee(Instruction& i) const;
};
