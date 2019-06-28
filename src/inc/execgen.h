#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"

#include <vector>
#include <set>

using namespace std;
using namespace llvm;

class ExecGen {
    private:
        static ExecGen* instance;
        set<Instruction*> instrumenteds;

        ExecGen();
        bool insert_interface() const;
        void insert_fuzz_to_tests (set<Function*> srcs) const;
        void insert_skip_to_tests (set<Function*> srcs) const;

    public:
        static ExecGen* get();
        void generate();
};
