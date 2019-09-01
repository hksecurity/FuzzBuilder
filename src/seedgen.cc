#include "seedgen.h"
#include "logger.h"
#include "loader.h"
#include "irreader.h"
#include "irwriter.h"

SeedGen* SeedGen::instance = nullptr;

SeedGen::SeedGen() {
}

void SeedGen::insert_collect_to_targets (set<Function*> srcs) const {
    for(auto e : srcs) {
        IRWriter writer = IRWriter(e);
        writer.collect();
    }
}

SeedGen* SeedGen::get() {
    if(instance == nullptr) {
        instance = new SeedGen();
    }
    return instance;
}

void SeedGen::generate() {
    if(!Loader::get()->load()) {
        Logger::get()->log(ERROR, "LLVM Module load failed.");
        return;
    }

    set<Function*> targets;
    for(size_t i = 0; i < Loader::get()->get_module_size(); ++i) {
        IRReader reader = IRReader(&Loader::get()->get_module(i));
        for(auto e : reader.get_functions_to_collect()) {
            targets.insert(e);
        }
    }

    size_t cnt = 1;
    for(auto e : targets) {
        Logger::get()->log(INFO, "Identified Target Functions #" +
            to_string(cnt++) + " " + string(e->getName()));
    }

    if(targets.size() == 0) {
        Logger::get()->log(WARN, "No Target Found.");
        return;
    }

    insert_collect_to_targets(targets);

    Loader::get()->dump();
}
