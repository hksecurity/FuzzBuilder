#include "logger.h"

#include "llvm/IR/Module.h"

#include <vector>

using namespace std;
using namespace llvm;

class Loader {
    private:
        static Loader* instance;
        vector<pair<string,Module*>> modules;

        Loader();
        ~Loader();
        void clean();
        Module* load_file(const string path) const;

    public:
        static Loader* get();
        size_t get_module_size() const;
        Module& get_module(const size_t);
        Function* get_entry_function();
        Module* get_entry_module() const;
        string get_unique_global_variable_name(const string) const;

        bool load();

        void dump() const;
        void called() const;
        void print(log_level level) const;
};
