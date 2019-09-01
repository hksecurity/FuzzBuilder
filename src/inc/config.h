#include "logger.h"

#include "rapidjson/document.h"

#include "llvm/IR/Function.h"

#include <string>
#include <vector>

using namespace std;
using namespace llvm;

class Config {
    private:
        class Target {
            private:
                string name;
                size_t fuzz;
                size_t len;

                void init(string, size_t, size_t);

            public:
                Target(string, size_t, size_t);
                Target(string, size_t);
                bool is_target(const string) const;

                string get_name() const;
                size_t get_fuzz() const;
                size_t get_size() const;
        };

        const string CONF_TARGET = "targets";
        const string CONF_TEST = "tests";
        const string CONF_SKIP = "skips";
        const string CONF_FILE = "files";

        static Config* instance;
        string path;
        string type;
        vector<string> tests;
        vector<string> skips;
        vector<string> files;
        vector<Target*> targets;

        Config();
        ~Config();

        Target* make_target(const rapidjson::Value& v) const;
        void clean();

    public:
        static Config* get();

        size_t get_fuzz(string name) const;
        size_t get_size(string name) const;

        vector<string> get_targets() const;
        vector<string> get_tests() const;
        vector<string> get_skips() const;
        vector<string> get_files() const;

        bool parse_cmd(int argc, char* argv[]);
        bool parse_conf();

        bool is_exec() const;

        void print_usage() const;
        void print(log_level level) const;
};
