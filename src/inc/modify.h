#ifndef __MODIFY_H__
#define __MODIFY_H__

#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalVariable.h"

#include <set>

using namespace llvm;
using namespace std;

class Modify {
    private:
        const string global_buffer_prefix = "autofuzz_buffer";
        const string global_size_prefix = "autofuzz_size";
        string global_buffer;
        string global_size;

        GlobalVariable* get_global_buffer(Module&, const bool);
        GlobalVariable* get_global_size(Module&, const bool);
        Function* get_read_function(Module&);
        Function* get_calloc_function(Module&);
        Function* get_free_function(Module&);
        Function* get_memcpy_function(Module&);
        set<Function*> collect_constructors(Module&);
        void set_argument(Instruction&, Value&, size_t);
        Function* get_called_function(Instruction&);

    public:
        Modify();
        void generate_interface(Function&);
        void generate_input(Instruction&);
        void replace_input(Function&);

        void remove_function(Function&);
        void remove_with_all_users(Value*);
        void clean_function(Function&);
        void erase_test(Function&);
        size_t erase_assert(Function&);
        set<Function*> erase_test_internal(Value&, const set<Function*>);
        set<Value*> last_user(Value&, set<Function*>, const Function&);
};

#endif
