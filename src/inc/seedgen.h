#include "llvm/IR/Function.h"

#include <set>

using namespace std;
using namespace llvm;

class SeedGen {
    private:
        static SeedGen* instance; 
        set<Function*> instrumenteds;

        SeedGen();
        void insert_collect_to_targets (set<Function*> srcs) const;

    public:
        static SeedGen* get();
        void generate();
};
