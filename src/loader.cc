#include "loader.h"
#include "config.h"

#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/CBindingWrapping.h"
#include "llvm-c/BitReader.h"
#include "llvm-c/BitWriter.h"
#include "llvm-c/Core.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"

using namespace llvm;

#include <iostream>

Loader* Loader::instance = nullptr;

Loader::Loader() {
}

Loader::~Loader() {
    clean();
}

void Loader::clean() {
    for(auto module : modules) {
        LLVMDisposeModule(reinterpret_cast<LLVMModuleRef>(module.second));
    }
    modules.clear();
}

Module* Loader::load_file(const string path) const {
    LLVMMemoryBufferRef memoryBuffer;
    char* message = nullptr;

    if(LLVMCreateMemoryBufferWithContentsOfFile(path.c_str(), &memoryBuffer, &message) != 0) {
        free(message);
        Logger::get()->log(ERROR, "[" + path + "] Failed to load");
        return nullptr;
    }

    LLVMModuleRef moduleRef;
    if(LLVMParseBitcode2(memoryBuffer, &moduleRef) != 0) {
        free(message);
        Logger::get()->log(ERROR, "[" + path + "] Failed to load");
        return nullptr;
    }

    LLVMDisposeMemoryBuffer(memoryBuffer);
    return unwrap(moduleRef);
}

Loader* Loader::get() {
    if(instance == nullptr) {
        instance = new Loader();
    }

    return instance;
}

size_t Loader::get_module_size() const {
    return modules.size();
}

Module& Loader::get_module(const size_t idx) {
    if(idx >= get_module_size()) {
        assert(false);
    }

    return *(modules[idx].second);
}

Function* Loader::get_entry_function() {
    for(auto module : modules) {
        Module* ptr = module.second;
        Function* entry = ptr->getFunction("main");
        if(entry == nullptr) {
            continue;
        }
        return entry;
    }

    assert(false);
}

Module* Loader::get_entry_module() const {
    Module* ret;
    for(auto e : this->modules) {
        Module* m = e.second;
        Function* entry = m->getFunction("main");
        if(entry == nullptr) {
            continue;
        }
        ret = m;
    }
    return ret;
}

string Loader::get_unique_global_variable_name(const string prefix) const {
    size_t cnt = 0;
    while(1) {
        string name = prefix;
        if(cnt != 0) {
            name += "_" + to_string(cnt);
        }
        cnt += 1;

        bool found = false;
        for(auto module : modules) {
            Module* ptr = module.second;
            if(ptr->getGlobalVariable(name) != nullptr) {
                found = true;
                break;
            }
        }

        if(!found) {
            return name;
        }
    }

    assert(false);
    return "";
}

bool Loader::load() {
    vector<string> files = Config::get()->get_files();
    for(auto e : files) {
        Module* lm = load_file(e);
        if (lm == nullptr) {
            this->clean();
            return false;
        }
        this->modules.push_back(pair<string, Module*>(
            e, lm));
    }

    this->print(DEBUG);
    return true;
}


void Loader::dump() const {
    for(auto module : modules) {
        string path = module.first + ".mod.bc";
        if(LLVMWriteBitcodeToFile(reinterpret_cast<LLVMModuleRef>(module.second), path.c_str()) != 0) {
            assert(false);
        }
        Logger::get()->log(INFO, module.first + " was modified to " + path);
    }
}

void Loader::called() const {
    vector<Function*> functions;
    for(auto module : modules) {
        for(Function &f : *(module.second)) {
            for(BasicBlock &b : f) {
                for(Instruction &i : b) {
                    if(isa<CallInst>(&i)) {
                        CallInst* t = dyn_cast<CallInst>(&i);
                        Function* c = t->getCalledFunction();
                        if(c == nullptr) {
                            continue;
                        }
                        if(c->size() != 0) {
                            continue;
                        }
                        if(find(functions.begin(), functions.end(), c) != functions.end()) {
                            continue;
                        }
                        functions.push_back(c);
                    }
                    else if(isa<InvokeInst>(&i)) {
                        InvokeInst* t = dyn_cast<InvokeInst>(&i);
                        Function* c = t->getCalledFunction();
                        if(c == nullptr) {
                            continue;
                        }
                        if(c->size() != 0) {
                            continue;
                        }
                        if(find(functions.begin(), functions.end(), c) != functions.end()) {
                            continue;
                        }
                        functions.push_back(c);
                    }
                }
            }
        }
    }
    for(auto e : functions) {
        cerr << string(e->getName()) << "\n";
    }
}

void Loader::print(log_level level) const {
    Logger::get()->log(level, "== Loader ===================");
    size_t i = 1;
    for(auto module : modules) {
        Logger::get()->log(level,
            "Loaded Module #" + to_string(i++) + " " + string(module.second->getName()));
    }
    Logger::get()->log(level, "=============================");
}
