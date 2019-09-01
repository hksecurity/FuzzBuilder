#include "irwriter.h"
#include "loader.h"
#include "irreader.h"
#include "config.h"
#include "irfreader.h"
#include "irireader.h"

#include "llvm/IR/IRBuilder.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

string IRWriter::GLOBAL_BUFFER = "";
string IRWriter::GLOBAL_SIZE = "";
set<Instruction*> IRWriter::MODIFIED;

GlobalVariable* IRWriter::get_global_buffer(Module& m, const bool is_user) {
    if(GLOBAL_BUFFER.empty()) {
        GLOBAL_BUFFER = Loader::get()->get_unique_global_variable_name(GLOBAL_BUFFER_PREFIX);
    }

    GlobalVariable* gv = m.getGlobalVariable(GLOBAL_BUFFER); 
    if(gv == nullptr) {
        gv = dyn_cast<GlobalVariable>(
            m.getOrInsertGlobal(GLOBAL_BUFFER, Type::getInt8PtrTy(m.getContext())));
        if(is_user) {
            gv->setLinkage(GlobalValue::ExternalLinkage);
        }
        else {
            gv->setLinkage(GlobalValue::CommonLinkage);
            gv->setInitializer(ConstantPointerNull::get(
               PointerType::get(Type::getInt8Ty(m.getContext()), 0)));
        }
    }

    return gv;
}

GlobalVariable* IRWriter::get_global_size(Module& m, const bool is_user) {
    if(GLOBAL_SIZE.empty()) {
        GLOBAL_SIZE = Loader::get()->get_unique_global_variable_name(GLOBAL_SIZE_PREFIX);
    }

    GlobalVariable* gv = m.getGlobalVariable(GLOBAL_SIZE); 
    if(gv == nullptr) {
        gv = dyn_cast<GlobalVariable>(
            m.getOrInsertGlobal(GLOBAL_SIZE, Type::getInt32Ty(m.getContext())));
        if(is_user) {
            gv->setLinkage(GlobalValue::ExternalLinkage);
        }
        else {
            gv->setLinkage(GlobalValue::CommonLinkage);
            gv->setInitializer(ConstantInt::get(Type::getInt32Ty(m.getContext()), 0));
        }
    }

    return gv;
}

Function* IRWriter::get_read_function(Module& m) {
    vector<Type*> ptys = {
        Type::getInt32Ty(m.getContext()),
        Type::getInt8PtrTy(m.getContext()),
        Type::getInt32Ty(m.getContext())
    };

    Type* rty = Type::getInt32Ty(m.getContext());

    FunctionType* fty = FunctionType::get(rty, ptys, false);

    return dyn_cast<Function>(m.getOrInsertFunction("read", fty));
}

Function* IRWriter::get_calloc_function(Module& m) {
    vector<Type*> ptys = {
        Type::getInt32Ty(m.getContext()),
        Type::getInt32Ty(m.getContext())
    };

    Type* rty = Type::getInt8PtrTy(m.getContext());

    FunctionType* fty = FunctionType::get(rty, ptys, false);

    return dyn_cast<Function>(m.getOrInsertFunction("calloc", fty));
}

Function* IRWriter::get_free_function(Module& m) {
    vector<Type*> ptys = {
        Type::getInt8PtrTy(m.getContext())
    };

    Type* rty = Type::getVoidTy(m.getContext());

    FunctionType* fty = FunctionType::get(rty, ptys, false);

    return dyn_cast<Function>(m.getOrInsertFunction("free", fty));
}

Function* IRWriter::get_memcpy_function(Module& m) {
    vector<Type*> ptys = {
        Type::getInt8PtrTy(m.getContext()),
        Type::getInt8PtrTy(m.getContext()),
        Type::getInt32Ty(m.getContext()),
        Type::getInt32Ty(m.getContext()),
        Type::getInt1Ty(m.getContext())
    };

    Type* rty = Type::getVoidTy(m.getContext());
    FunctionType *fty = FunctionType::get(rty, ptys, false);
    return dyn_cast<Function>(m.getOrInsertFunction("llvm.memcpy.p0i8.p0i8.i32", fty));
}

Function* IRWriter::get_open_function(Module& m) {
    vector<Type*> ptys = {
        Type::getInt8PtrTy(m.getContext()),
        Type::getInt32Ty(m.getContext()),
    };

    Type* rty = Type::getInt32Ty(m.getContext());
    FunctionType *fty = FunctionType::get(rty, ptys, true);
    return dyn_cast<Function>(m.getOrInsertFunction("open64", fty));
}

Function* IRWriter::get_flock_function(Module& m) {
    vector<Type*> ptys = {
        Type::getInt32Ty(m.getContext()),
        Type::getInt32Ty(m.getContext())
    };

    Type* rty = Type::getInt32Ty(m.getContext());
    FunctionType* fty = FunctionType::get(rty, ptys, false);
    return dyn_cast<Function>(m.getOrInsertFunction("flock", fty));
}

Function* IRWriter::get_write_function(Module &m) {
    vector<Type*> ptys = {
        Type::getInt32Ty(m.getContext()),
        Type::getInt8PtrTy(m.getContext()),
        Type::getInt32Ty(m.getContext())
    };

    Type* rty = Type::getInt32Ty(m.getContext());
    FunctionType* fty = FunctionType::get(rty, ptys, false);
    return dyn_cast<Function>(m.getOrInsertFunction("write", fty));
}

Function* IRWriter::get_close_function(Module& m) {
    vector<Type*> ptys = {
        Type::getInt32Ty(m.getContext())
    };

    Type* rty = Type::getInt32Ty(m.getContext());
    FunctionType* fty = FunctionType::get(rty, ptys, false);
    return dyn_cast<Function>(m.getOrInsertFunction("close", fty));
}

Function* IRWriter::get_strlen_function(Module& m) {
    vector<Type*> ptys = {
        Type::getInt8PtrTy(m.getContext()),
    };

    Type* rty = Type::getInt32Ty(m.getContext());
    FunctionType* fty = FunctionType::get(rty, ptys, false);
    return dyn_cast<Function>(m.getOrInsertFunction("strlen", fty));
}

void IRWriter::set_argument(Instruction& i, Value& v, size_t idx) {
    if(isa<CallInst>(&i)) {
        CallInst* C = dyn_cast<CallInst>(&i);
        C->setArgOperand(idx, &v);
    }
    else if(isa<InvokeInst>(&i)) {
        InvokeInst* IN = dyn_cast<InvokeInst>(&i);
        IN->setArgOperand(idx, &v);
    }
    else {
        assert(false);
    }
}

bool IRWriter::is_modified(Instruction& i) {
    if(find(MODIFIED.begin(), MODIFIED.end(), &i) != MODIFIED.end()) {
        return true;
    }
    return false;
}

void IRWriter::set_modified(Instruction& i) {
    MODIFIED.insert(&i);
}

IRWriter::IRWriter(Function* f) {
    this->f = f;
}

bool IRWriter::interface() {
    BasicBlock& entry1 = this->f->getEntryBlock();
    Instruction& inst = *(entry1.getFirstInsertionPt());
    BasicBlock* link = entry1.splitBasicBlock(&inst);
    entry1.begin()->eraseFromParent();
    IRBuilder<> builder(&entry1);
    LLVMContext& ctx = this->f->getContext();
    Module& module = *(this->f->getParent());

    GlobalVariable& glob_buf = *(get_global_buffer(module, false));
    GlobalVariable& glob_size = *(get_global_size(module, false));

    BasicBlock* entry2 = BasicBlock::Create(ctx, "", this->f);
    BasicBlock* entry3 = BasicBlock::Create(ctx, "", this->f);
    BasicBlock* entry4 = BasicBlock::Create(ctx, "", this->f);
    BasicBlock* entry5 = BasicBlock::Create(ctx, "", this->f);

    Type* bufferType = ArrayType::get(Type::getInt8Ty(ctx), 4096);
    Value* tmp = builder.CreateAlloca(bufferType);
    Value* n0 = builder.CreateInBoundsGEP(bufferType, tmp,
        { builder.getInt32(0), builder.getInt32(0) });
    Value* call = builder.CreateCall(get_read_function(module),
        { builder.getInt32(0), n0, builder.getInt32(4096) }); 
    Value* cmp = builder.CreateICmpEQ(call, builder.getInt32(-1));
    builder.CreateCondBr(cmp, link, entry2);

    builder.SetInsertPoint(entry2);

    Value* add = builder.CreateNSWAdd(call, builder.getInt32(1));
    Value* call1 = builder.CreateCall(get_calloc_function(module),
        { add, builder.getInt32(1) });
    builder.CreateStore(call1, &glob_buf);
    builder.CreateCall(get_memcpy_function(module),
        { call1, n0, call, builder.getInt32(1), builder.getInt1(0) });
    Value* n1 = builder.CreateLoad(&glob_size);
    Value* add3 = builder.CreateAdd(n1, call); 
    builder.CreateStore(add3, &glob_size);
    Value* cmp427 = builder.CreateICmpEQ(call, builder.getInt32(4096));
    builder.CreateCondBr(cmp, entry3, link);

    builder.SetInsertPoint(entry3);
    builder.CreateBr(entry4);

    builder.SetInsertPoint(entry4);
    Value* call6 = builder.CreateCall(get_read_function(module),
        { builder.getInt32(0), n0, builder.getInt32(4096) });
    Value* cond = builder.CreateICmpEQ(call6, builder.getInt32(-1));
    builder.CreateCondBr(cond, link, entry5);

    builder.SetInsertPoint(entry5);
    Value* n2 = builder.CreateLoad(&glob_size);
    Value* add9 = builder.CreateAdd(call6, builder.getInt32(1));
    Value* add10 = builder.CreateAdd(add9, n2);
    Value* call11 = builder.CreateCall(get_calloc_function(module),
        { add10, builder.getInt32(1) });
    Value* n3 = builder.CreateLoad(&glob_buf);
    builder.CreateCall(get_memcpy_function(module),
        { call11, n3, n2, builder.getInt32(1), builder.getInt1(0) });
    Value* add_ptr = builder.CreateInBoundsGEP(Type::getInt8Ty(ctx), call11, n2);
    builder.CreateCall(get_memcpy_function(module),
        { add_ptr, n0, call6, builder.getInt32(1), builder.getInt1(0) });
    builder.CreateCall(get_free_function(module), { n3 });
    builder.CreateStore(call11, &glob_buf);
    Value* n4 = builder.CreateLoad(&glob_size);
    Value* add13 = builder.CreateAdd(n4, call6);
    builder.CreateStore(add13, &glob_size);
    Value* cmp4 = builder.CreateICmpEQ(call6, builder.getInt32(4096));
    builder.CreateCondBr(cmp4, entry4, link);
    return true; 
}

void IRWriter::fuzz() {
    IRReader reader = IRReader(this->f->getParent());

    set<Instruction*> srcs =
        reader.get_target_instructions(string(this->f->getName()));

    size_t cnt = 1;
    for(auto e : srcs) {
        if(this->is_modified(*e)) {
            continue;
        }

        Function* callee = IRIReader(e).get_callee();
        if(callee == nullptr) {
            continue;
        }
        size_t fuzz_slot = Config::get()->get_fuzz(string(callee->getName()));
        if(fuzz_slot == 0) {
            Logger::get()->log(WARN, "Unexpected Target (" + string(callee->getName()));
            continue;
        }
        size_t len_slot = Config::get()->get_size(string(callee->getName()));

        IRBuilder<> builder(e);

        GlobalVariable* gv_b = get_global_buffer(*e->getModule(), true);
        GlobalVariable* gv_s = get_global_size(*e->getModule(), true);

        Value* n0 = builder.CreateLoad(gv_b);
        this->set_argument(*e, *n0, fuzz_slot - 1);
        if(len_slot != 0) {
            Value* n1 = builder.CreateLoad(gv_s);
            this->set_argument(*e, *n1, len_slot - 1);
        }

        this->set_modified(*e);
        Logger::get()->log(DEBUG, "Fuzz Instrumented #" +
            to_string(cnt++) + " at " + string(this->f->getName()));
    }

    if(!this->f->getFunctionType()->getReturnType()->isVoidTy()) {
        return;
    }

    IRFReader freader = IRFReader(this->f);
    set<Instruction*> asserts = freader.get_asserts();
    cnt = 1;
    for(auto e : asserts) {
        Instruction* i = ReturnInst::Create(this->f->getContext(), nullptr, e);
        e->eraseFromParent();
        i = i->getNextNode();
        if(i != nullptr && isa<UnreachableInst>(i)) {
            i->eraseFromParent();
        }
        Logger::get()->log(DEBUG, "Assert erased #" +
            to_string(cnt++) + " at " + string(this->f->getName()));
    }
}

bool IRWriter::skip() {
    Type* ty = this->f->getFunctionType()->getReturnType();
    if(!ty->isVoidTy() && !ty->isIntegerTy(32)) {
        return false;
    }

    while(this->f->size() != 0) {
        for(BasicBlock& b : *(this->f)) {
            b.dropAllReferences();
            for(Instruction& i : b) {
                i.replaceAllUsesWith(UndefValue::get(i.getType()));
                i.eraseFromParent();
                break;
            }
        }
        this->f->begin()->eraseFromParent();
    }

    BasicBlock* basicblock = BasicBlock::Create(this->f->getContext(),
        "entry", this->f);
    IRBuilder<> builder(basicblock);
    if(ty->isVoidTy()) {
        builder.CreateRetVoid();
    }
    else if(ty->isIntegerTy(32)) {
        builder.CreateRet(builder.getInt32(0));
    }

    Logger::get()->log(INFO, "Skip Instrumented at " + string(this->f->getName()));
    return true;
}

void IRWriter::collect() {

    if(this->f->size() == 0) {
        return;
    }

    BasicBlock& entry1 = this->f->getEntryBlock();
    Instruction& inst = *(entry1.getFirstInsertionPt());
    BasicBlock* link = entry1.splitBasicBlock(&inst);
    entry1.begin()->eraseFromParent();
    IRBuilder<> builder(&entry1);
    LLVMContext& ctx = this->f->getContext();
    Module& module = *(this->f->getParent());

    BasicBlock* entry2 = BasicBlock::Create(ctx, "", this->f);

    Value* buffer = this->f->arg_begin() +
        (Config::get()->get_fuzz(string(this->f->getName())) - 1);
    Value* size = nullptr;

    if(Config::get()->get_size(string(this->f->getName())) != 0) {
        size = this->f->arg_begin() +
            (Config::get()->get_size(string(this->f->getName())) - 1 );
    } else {
        size = builder.CreateCall(get_strlen_function(module),
            { buffer });
    }

    Value* fd = builder.CreateAlloca(Type::getInt32Ty(module.getContext()));
    Value* path = builder.CreateGlobalString(COLLECT_PATH);
    Value* func_name = builder.CreateGlobalString(string(this->f->getName()));
    Value* func_name_size = builder.getInt32(string(this->f->getName()).size());
    Value* splitter = builder.CreateGlobalString(SPLITTER);
    Value* splitter_size = builder.getInt32(SPLITTER.size());
    Value* newline = builder.CreateGlobalString("\n");

    Value* cmp = builder.CreateICmpUGT(size, builder.getInt32(1));
    builder.CreateCondBr(cmp, entry2, link);

    builder.SetInsertPoint(entry2);

    Function* func = get_open_function(module);

    Value* call = builder.CreateCall(func,
        { builder.CreateInBoundsGEP(path, {builder.getInt32(0), builder.getInt32(0)}),
            builder.getInt32(1089), builder.getInt32(420) });
    Value* call1 = builder.CreateCall(get_flock_function(module),
        { call, builder.getInt32(2) });
    Value* call2 = builder.CreateCall(get_write_function(module),
        { call, builder.CreateInBoundsGEP(func_name, {builder.getInt32(0), builder.getInt32(0)}),
            func_name_size });
    Value* call3 = builder.CreateCall(get_write_function(module),
        { call, builder.CreateInBoundsGEP(newline, {builder.getInt32(0), builder.getInt32(0)}),
            builder.getInt32(1) });
    Value* call4 = builder.CreateCall(get_write_function(module),
        { call, buffer, size });
    Value* call5 = builder.CreateCall(get_write_function(module),
        { call, builder.CreateInBoundsGEP(newline, {builder.getInt32(0), builder.getInt32(0)}), builder.getInt32(1) });
    Value* call6 = builder.CreateCall(get_write_function(module),
        { call, builder.CreateInBoundsGEP(splitter, {builder.getInt32(0), builder.getInt32(0)}), splitter_size });
    Value* call7 = builder.CreateCall(get_flock_function(module),
        { call, builder.getInt32(8)} );
    Value* call8 = builder.CreateCall(get_close_function(module),
        { call });
    builder.CreateBr(link);

    Logger::get()->log(INFO, "Collect Instrumented at " + string(this->f->getName()));
}
