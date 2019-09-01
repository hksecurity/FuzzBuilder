#include "irreader.h"
#include "config.h"
#include "irireader.h"

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/Support/raw_ostream.h"

#include <iostream>
using namespace std;

set<Function*> IRReader::get_gtest_functions() const {
    set<Function*> ret;

    for(Function& f : *this->m) {
        if(!f.getFunctionType()->getReturnType()->isVoidTy()) {
            continue;
        }

        DISubprogram *subprogram = f.getSubprogram();
        if(subprogram == nullptr) {
            if(f.getName().find("TestBody") == string::npos) {
                continue;
            }
        }
        else {
            if(subprogram->getName() != "TestBody") {
                continue;
            }
        }
        ret.insert(&f);
    }
    return ret;
}

set<Instruction*> IRReader::get_calls(Function& f) const {
    set<Instruction*> ret;

    for(BasicBlock& b : f) {
        for(Instruction& i : b) {
            if(isa<CallInst>(&i) || isa<InvokeInst>(&i)) {
                ret.insert(&i);
            }
        }
    }

    return ret;
}

set<Function*> IRReader::get_test_functions() const {
    set<Function*> ret;
    vector<string> test_opts = Config::get()->get_tests();

    if(test_opts.size() == 0) {
        ret = get_gtest_functions();
    } 
    else {
        for(Function& f : *(this->m)) {
            if(this->is_test(f)) {
                ret.insert(&f);
            }
        }
    }

    return ret;
}

set<Function*> IRReader::get_target_functions() const {
    set<Function*> ret;
    vector<string> targets = Config::get()->get_targets();

    for(Function& f : *(this->m)) {
        if(find(targets.begin(), targets.end(), string(f.getName())) != targets.end()) {
            ret.insert(&f);
        }
    }

    return ret;
}

/*
Function* IRReader::get_callee(Instruction &i) const {
    if(isa<CallInst>(&i)) {
        CallInst* ci = dyn_cast<CallInst>(&i);
        if(ci == nullptr) {
            return nullptr;
        }

        Function* callee = ci->getCalledFunction();
        if(callee == nullptr) {
            return nullptr;
        }

        return callee;
    } 
    else if(isa<InvokeInst>(&i)) {
        InvokeInst* ii = dyn_cast<InvokeInst>(&i);
        if(ii == nullptr) {
            return nullptr;
        }

        Function* callee = ii->getCalledFunction();
        if(callee == nullptr) {
            return nullptr;
        }

        return callee;
    }

    return nullptr;
}
*/

bool IRReader::is_target(Function& f) const {
    vector<string> targets = Config::get()->get_targets();
    set<Function*> callees = this->get_callees(f);
    for(auto callee : callees) {
        for(auto target : targets) {
            if (string(callee->getName()) == target) {
                return true;
            }
        }
    }
    return false;
}

bool IRReader::is_target(Instruction& i) const {
    vector<string> targets = Config::get()->get_targets();
    Function* callee = IRIReader(&i).get_callee();
    if(callee == nullptr) {
        return false;
    }

    if(find(targets.begin(), targets.end(), string(callee->getName()))
        != targets.end()) {
        return true;
    }
    return false;
}

bool IRReader::is_test(Function& f) const {
    vector<string> test_opts = Config::get()->get_tests();

    string n = string(f.getName());
    for(auto test_opt : test_opts) {
        if(test_opt.size() > n.size()) {
            continue;
        }
        if(n.compare(0, test_opt.size(), test_opt) == 0) {
            return true;
        }
    }

    return false;
}

IRReader::IRReader(Module* m) {
    this->m = m; 
}

set<Function*> IRReader::get_functions_to_fuzz() const {
    set<Function*> ret = this->get_test_functions();

    size_t cnt = 1;
    for(auto e : ret) {
        Logger::get()->log(DEBUG, "Test Function #" +
            to_string(cnt++) + " " + string(e->getName()));
    }

    set<Function*> tmps;
    for(auto e : ret) {
        if (!this->is_target(*e)) {
            tmps.insert(e);
        }
    }
    cnt = 1;
    for(auto e : tmps) {
        Logger::get()->log(DEBUG, "No Target Function #" +
            to_string(cnt++) + " " + string(e->getName()));
    }
    for(auto e : tmps) {
        ret.erase(e);
    }

    tmps.clear();
    vector<string> skip_opts = Config::get()->get_skips();
    for(auto e : ret) {
        string n = string(e->getName());
        if(find(skip_opts.begin(), skip_opts.end(), n) != skip_opts.end()) {
            tmps.insert(e);
        }
    }
    cnt = 1;
    for(auto e : tmps) {
        Logger::get()->log(DEBUG, "Skip Function #" +
            to_string(cnt++) + " " + string(e->getName()));
    }
    for(auto e : tmps) {
        ret.erase(e);
    }

    return ret;
}

set<Function*> IRReader::get_functions_to_remove() const {
    set<Function*> ret = this->get_test_functions();
    set<Function*> targets = this->get_functions_to_fuzz();

    set<Function*> tmps;
    for(auto e : ret) {
        if(find(targets.begin(), targets.end(), e) != targets.end()) {
            tmps.insert(e);
        }
    }
    for(auto e : tmps) {
        ret.erase(e);
    }

    return ret;
}

set<Function*> IRReader::get_functions_to_collect() const {
    return  this->get_target_functions();
}

set<Instruction*> IRReader::get_target_instructions(string n) const {
    set<Instruction*> ret;

    Function* f = this->m->getFunction(n);
    if(f == nullptr) {
        return ret;
    } 

    set<Instruction*> calls = this->get_calls(*f);
    for(auto e : calls) {
        if(!this->is_target(*e)) {
            continue;
        }
        ret.insert(e);
    }

    return ret;
}

set<Function*> IRReader::get_callees(Function& f) const {
    set<Function*> ret;

    set<Instruction*> calls = this->get_calls(f);
    for(auto e : calls) {
        Function* callee = IRIReader(e).get_callee();
        if(callee != nullptr) {
            ret.insert(callee);
        }
    }

    return ret;
}
