#ifndef __IRWRITER_H__
#define __IRWRITER_H__

#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"

#include <set>

using namespace llvm;
using namespace std;

class IRWriter {
    private:
        static string GLOBAL_BUFFER;
        static string GLOBAL_SIZE;
        static set<Instruction*> MODIFIED;
        const string GLOBAL_BUFFER_PREFIX = "fuzzbuilder_buffer";
        const string GLOBAL_SIZE_PREFIX = "fuzzbuilder_size";
        Function* f;

        GlobalVariable* get_global_buffer(Module& m, const bool is_user);
        GlobalVariable* get_global_size(Module& m, const bool is_user);
        Function* get_read_function(Module& m);
        Function* get_calloc_function(Module& m);
        Function* get_free_function(Module& m);
        Function* get_memcpy_function(Module& m);

        void set_argument(Instruction& i, Value& v, size_t idx);

        bool is_modified(Instruction& i);
        void set_modified(Instruction& i);

    public:
        IRWriter(Function* f);
        bool interface();
        void fuzz();
        bool skip();
};

#endif
