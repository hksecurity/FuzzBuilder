#include "config.h"

#include "rapidjson/filereadstream.h"

#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/DebugInfoMetadata.h"

#include <algorithm>
#include <assert.h>
#include <iostream>

void Config::Target::init(string name, size_t fuzz, size_t len) {
    this->name = name;
    this->fuzz = fuzz;
    this->len = len;
}

Config::Target::Target(string name, size_t fuzz) {
    init(name, fuzz, 0);
}

Config::Target::Target(string name, size_t fuzz, size_t len) {
    init(name, fuzz, len);
}

bool Config::Target::is_target(const string src) const {
    if(name.compare(src) == 0) {
        return true;
    }
    return false;
}

string Config::Target::get_name() const {
    return this->name;
}

size_t Config::Target::get_fuzz() const {
    return this->fuzz;
}

size_t Config::Target::get_size() const {
    return this->len;
}

Config* Config::instance = nullptr;

Config::Target* Config::make_target(const rapidjson::Value& v) const {
    Target* t = nullptr;

    if(!v.IsArray()) {
        return t;
    }

    string name = "";
    size_t fuzz = 0;
    size_t len = 0;

    if(v.Size() > 0 && v[0].IsString()) {
        name = v[0].GetString();
    }

    if(v.Size() > 1 && v[1].IsInt()) {
        fuzz = v[1].GetInt();
    }
    if(v.Size() > 2 && v[2].IsInt()) {
        len = v[2].GetInt();
    }

    if (name.empty() || fuzz == 0) {
        return t;
    }

    t = new Target(name, fuzz, len);
    return t;
}

void Config::clean() {
    while(this->targets.size() != 0) {
        delete this->targets.back();
        this->targets.pop_back();
    }
    this->tests.clear();
    this->skips.clear();
    this->files.clear();
}

Config::Config() {
}

Config::~Config() {
    this->clean();
}

Config* Config::get() {
    if(instance == nullptr) {
        instance = new Config();
    }
    return instance;
}

size_t Config::get_fuzz(string name) const {
    for(auto e : this->targets) {
        if (e->get_name() == name) {
            return e->get_fuzz();
        }
    }
    return 0;
}

size_t Config::get_size(string name) const {
    for(auto e : this->targets) {
        if (e->get_name() == name) {
            return e->get_size();
        }
    }
    return 0;
}

vector<string> Config::get_targets() const {
    vector<string> ret;

    for(auto target : targets) {
        ret.push_back(target->get_name());
    }

    return ret;
}

vector<string> Config::get_tests() const {
    return this->tests;
}

vector<string> Config::get_skips() const {
    return this->skips;
}

vector<string> Config::get_files() const {
    return this->files;
}

bool Config::parse_cmd(int argc, char* argv[]) {
    if (argc >= 3) {
        this->type = string(argv[1]);
        this->path = string(argv[2]);

        if (this->type != "exec" && this->type != "seed") {
            return false;
        }

        return true;
    }
    return false;
}

bool Config::parse_conf() {
    static bool PARSE_CONF_FLAG = false;
    if (PARSE_CONF_FLAG == true) {
        return false;
    }

    FILE* filep;
    char buffer[0xFFFF];
    rapidjson::Document d;
    filep = fopen((this->path).c_str(), "rb");
    if(filep == nullptr) {
        Logger::get()->log(ERROR, "[" + this->path + "] File Not Found");
        return false;
    }
    rapidjson::FileReadStream frs(filep, buffer, sizeof(buffer));
    d.ParseStream(frs);
    fclose(filep);

    if(!d.IsObject()) {
        Logger::get()->log(ERROR, "[" + this->path + "] Wrong Json Format");
        return false;
    }

    if(!d.HasMember(this->CONF_TARGET.c_str())) {
        Logger::get()->log(ERROR, "[" + this->CONF_TARGET + "] Mandatory Option Not Found");
        return false;
    }
    if(d[CONF_TARGET.c_str()].IsArray() && d[CONF_TARGET.c_str()][0].IsArray()) {
        for(rapidjson::SizeType i = 0; i < d[CONF_TARGET.c_str()].Size(); ++i) {
            Target* t = make_target(d[CONF_TARGET.c_str()][i]);
            if (t == nullptr) {
                Logger::get()->log(ERROR, "[" + CONF_TARGET + "] Invalid Configuration");
                this->clean();
                return false;
            }
            targets.push_back(t);
        }
    }

    if(d.HasMember(this->CONF_TEST.c_str())) {
        for(rapidjson::SizeType i = 0; i < d[CONF_TEST.c_str()].Size(); ++i) {
            if (!d[CONF_TEST.c_str()][i].IsString()) {
                Logger::get()->log(ERROR, "[" + CONF_TEST + "] Invalid Configuration");
                this->clean();
                return false;
            }
            tests.push_back(d[CONF_TEST.c_str()][i].GetString());
        }
    }

    if(d.HasMember(this->CONF_SKIP.c_str())) {
        for(rapidjson::SizeType i = 0; i < d[CONF_SKIP.c_str()].Size(); ++i) {
            if (!d[CONF_SKIP.c_str()][i].IsString()) {
                Logger::get()->log(ERROR, "[" + CONF_SKIP + "] Invalid Configuration");
                this->clean();
                return false;
            }
            skips.push_back(d[CONF_SKIP.c_str()][i].GetString());
        }
    }

    if(d.HasMember(this->CONF_FILE.c_str())) {
        for(rapidjson::SizeType i = 0; i < d[CONF_FILE.c_str()].Size(); ++i) {
            if (!d[CONF_FILE.c_str()][i].IsString()) {
                Logger::get()->log(ERROR, "[" + CONF_FILE + "] Invalid Configuration");
                this->clean();
                return false;
            }
            files.push_back(d[CONF_FILE.c_str()][i].GetString());
        }
    }

    this->print(DEBUG);
    return true;
}

bool Config::is_exec() const {
    if (this->type == "exec") {
        return true;
    }
    return false;
}

void Config::print_usage() const {
    Logger::get()->log(INFO, "./fuzzbuilder ${type}[exec|seed] ${config_path}");
}

void Config::print(log_level level) const {
    Logger::get()->log(level, "== Config ===================");

    size_t cnt = 1;
    for(auto e : this->targets) {
        string target = e->get_name() + " " + to_string(e->get_fuzz());
        if (e->get_size() == 0) {
            target += " None";
        }
        else {
            target += to_string(e->get_size());
        }
        Logger::get()->log(level, "TARGET #" + to_string(cnt++) + " " + target);
    }

    cnt = 1;
    for(auto test : tests) {
        Logger::get()->log(level, "TEST #" + to_string(cnt++) + " " + test);
    }

    cnt = 1;
    for(auto file : files) {
        Logger::get()->log(level, "FILE #" + to_string(cnt++) + " " + file);
    }

    Logger::get()->log(level, "=============================");

}
