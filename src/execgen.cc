#include "execgen.h"
#include "loader.h"
#include "config.h"
#include "irreader.h"
#include "irwriter.h"
#include "modify.h"

#include "llvm/IR/Module.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/DebugInfoMetadata.h"

#include <iostream>
#include <map>

ExecGen* ExecGen::instance = nullptr;

ExecGen::ExecGen() {
}

bool ExecGen::insert_interface() const {
    Function* entry = Loader::get()->get_entry_function();
    if (entry == nullptr) {
        Logger::get()->log(ERROR, "Entry Function Not Found."); 
        return false;
    }

    IRWriter writer = IRWriter(entry);
    if(!writer.interface()) {
        Logger::get()->log(ERROR, "Failed to insert interface");
        return false;
    }
    Logger::get()->log(INFO, "Interface inserted at [" + string(entry->getName()) +"]");
    return true;
}

void ExecGen::insert_fuzz_to_tests(set<Function*> srcs) const {
    for(auto e : srcs) {
        IRWriter writer = IRWriter(e);
        writer.fuzz();
    }
}

void ExecGen::insert_skip_to_tests(set<Function*> srcs) const {
    for(auto e : srcs) {
        IRWriter writer = IRWriter(e);
        if(!writer.skip()) {
            Logger::get()->log(WARN, "Skip Instrumented Failed [" + string(e->getName()) + "]");
        }
    }
}

ExecGen* ExecGen::get() {
    if(instance == nullptr) {
        instance = new ExecGen();
    }
    return instance;
}

void ExecGen::generate() {
    if(!Loader::get()->load()) {
        Logger::get()->log(ERROR, "LLVM Module load failed.");
        return;
    }

    set<Function*> targets;
    set<Function*> skips;
    for(size_t i = 0; i < Loader::get()->get_module_size(); ++i) {
        IRReader reader = IRReader(&Loader::get()->get_module(i));
        for(auto e : reader.get_functions_to_fuzz()) {
            targets.insert(e);
        }
        for(auto e : reader.get_functions_to_remove()) {
            skips.insert(e);
        }
    }

    size_t cnt = 1;
    for(auto e : targets) {
        Logger::get()->log(INFO, "Identified Target Functions #" +
            to_string(cnt++) + " " + string(e->getName()));
    }

    cnt = 1;
    for(auto e : skips) {
        Logger::get()->log(INFO, "Identified Functions to remove #" +
            to_string(cnt++) + " " + string(e->getName()));
    }

    if(targets.size() == 0) {
        Logger::get()->log(WARN, "No Target Found.");
        return;
    }

    if(!insert_interface()) {
        return;
    }

    insert_fuzz_to_tests(targets);
    insert_skip_to_tests(skips);

    Loader::get()->dump();
}
