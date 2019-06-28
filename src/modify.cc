#include "modify.h"
#include "loader.h"
#include "config.h"

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/GlobalValue.h"

#include <iostream>

using namespace std;

Modify::Modify() {}

GlobalVariable* Modify::get_global_buffer(Module& module, const bool isUser) {
    if(global_buffer.empty()) {
        global_buffer = Loader::get()->get_unique_global_variable_name(global_buffer_prefix);
    }

    GlobalVariable* gv = module.getGlobalVariable(global_buffer); 
    if(gv == nullptr) {
        gv = dyn_cast<GlobalVariable>(
            module.getOrInsertGlobal(global_buffer, Type::getInt8PtrTy(module.getContext())));
        if(isUser) {
            gv->setLinkage(GlobalValue::ExternalLinkage);
        }
        else {
            gv->setLinkage(GlobalValue::CommonLinkage);
            gv->setInitializer(ConstantPointerNull::get(
               PointerType::get(Type::getInt8Ty(module.getContext()), 0)));
        }
    }

    return gv;
}

GlobalVariable* Modify::get_global_size(Module& module, const bool isUser) {
    if(global_size.empty()) {
        global_size = Loader::get()->get_unique_global_variable_name(global_size_prefix);
    }

    GlobalVariable* gv = module.getGlobalVariable(global_size); 
    if(gv == nullptr) {
        gv = dyn_cast<GlobalVariable>(
            module.getOrInsertGlobal(global_size, Type::getInt32Ty(module.getContext())));
        if(isUser) {
            gv->setLinkage(GlobalValue::ExternalLinkage);
        }
        else {
            gv->setLinkage(GlobalValue::CommonLinkage);
            gv->setInitializer(ConstantInt::get(Type::getInt32Ty(module.getContext()), 0));
        }
    }

    return gv;
}

Function* Modify::get_read_function(Module& module) {
    vector<Type*> ptys = {
        Type::getInt32Ty(module.getContext()),
        Type::getInt8PtrTy(module.getContext()),
        Type::getInt32Ty(module.getContext())
    };

    Type* rty = Type::getInt32Ty(module.getContext());

    FunctionType* fty = FunctionType::get(rty, ptys, false);

    return dyn_cast<Function>(module.getOrInsertFunction("read", fty));
}

Function* Modify::get_calloc_function(Module& module) {
    vector<Type*> ptys = {
        Type::getInt32Ty(module.getContext()),
        Type::getInt32Ty(module.getContext())
    };

    Type* rty = Type::getInt8PtrTy(module.getContext());

    FunctionType* fty = FunctionType::get(rty, ptys, false);

    return dyn_cast<Function>(module.getOrInsertFunction("calloc", fty));
}

Function* Modify::get_free_function(Module& module) {
    vector<Type*> ptys = {
        Type::getInt8PtrTy(module.getContext())
    };

    Type* rty = Type::getVoidTy(module.getContext());

    FunctionType* fty = FunctionType::get(rty, ptys, false);

    return dyn_cast<Function>(module.getOrInsertFunction("free", fty));
}

Function* Modify::get_memcpy_function(Module& module) {
    vector<Type*> ptys = {
        Type::getInt8PtrTy(module.getContext()),
        Type::getInt8PtrTy(module.getContext()),
        Type::getInt32Ty(module.getContext()),
        Type::getInt32Ty(module.getContext()),
        Type::getInt1Ty(module.getContext())
    };

    Type* rty = Type::getVoidTy(module.getContext());
    FunctionType *fty = FunctionType::get(rty, ptys, false);
    return dyn_cast<Function>(module.getOrInsertFunction("llvm.memcpy.p0i8.p0i8.i32", fty));
}

set<Function*> Modify::collect_constructors(Module& M) {
    set<Function*> ret;

    GlobalVariable *GV = M.getGlobalVariable("llvm.global_ctors");
    if (!GV) {
        return ret;
    }
    if (!GV->hasUniqueInitializer()) {
        return ret;
    }
    if (!isa<ConstantAggregateZero>(GV->getInitializer())) {
        ConstantArray *CA = cast<ConstantArray>(GV->getInitializer());
        for (auto &V : CA->operands()) {
            if (isa<ConstantAggregateZero>(V)) {
                continue;
            }
        
            ConstantStruct *CS = cast<ConstantStruct>(V);
            if (isa<ConstantPointerNull>(CS->getOperand(1))) {
                continue;
            }
        
            if (!isa<Function>(CS->getOperand(1))) {
                return ret;
            }

            ConstantInt *CI = cast<ConstantInt>(CS->getOperand(0));
            if (CI->getZExtValue() != 65535) {
                return ret;
            }
        }
    }

    vector<Function*> ctors;
    ConstantArray *CA = cast<ConstantArray>(GV->getInitializer());
    ctors.reserve(CA->getNumOperands());
    for(auto &V : CA->operands()) {
        ConstantStruct *CS = cast<ConstantStruct>(V);
        ctors.push_back(dyn_cast<Function>(CS->getOperand(1)));
    }

    for(auto ctor : ctors) {
        for(BasicBlock& B : *ctor) {
            for(Instruction& I : B) {
                if(isa<CallInst>(&I)) {
                    CallInst& CI = *dyn_cast<CallInst>(&I);
                    if(CI.getCalledFunction() != nullptr) {
                        ret.insert(CI.getCalledFunction());
                    }
                }
            }
        }
    }

    return ret;
}

void Modify::set_argument(Instruction& I, Value& V, size_t idx) {
    if(isa<CallInst>(&I)) {
        CallInst* C = dyn_cast<CallInst>(&I);
        C->setArgOperand(idx, &V);
    }
    else if(isa<InvokeInst>(&I)) {
        InvokeInst* IN = dyn_cast<InvokeInst>(&I);
        IN->setArgOperand(idx, &V);
    }
    else {
        assert(false);
    }
}

Function* Modify::get_called_function(Instruction& instruction) {
    Function* ret = nullptr;
    if(!isa<CallInst>(&instruction) &&
        !isa<InvokeInst>(&instruction)) {
        return ret;
    }

    if(isa<CallInst>(&instruction)) {
        ret = dyn_cast<CallInst>(&instruction)->getCalledFunction();
    }
    else {
        ret = dyn_cast<InvokeInst>(&instruction)->getCalledFunction();
    }

    return ret;
}

void Modify::remove_function(Function& function) {
    clean_function(function);

    BasicBlock* basicblock = BasicBlock::Create(function.getContext(),
        "entry", &function);
    IRBuilder<> builder(basicblock);

    if(function.getFunctionType()->getReturnType()->isVoidTy()) {
        builder.CreateRetVoid();
    }
    else if(function.getFunctionType()->getReturnType()->isIntegerTy(32)) {
        builder.CreateRet(builder.getInt32(0));
    }
    else {
        cout << string(function.getName()) << "\n";
//        function.getFunctionType()->dump();
        assert(false);
    }
}

void Modify::clean_function(Function& function) {
    while(function.size() != 0) {
        for(BasicBlock& basicblock : function) {
            basicblock.dropAllReferences();
            for(Instruction& instruction : basicblock) {
                instruction.replaceAllUsesWith(UndefValue::get(instruction.getType()));
                instruction.eraseFromParent();
                break;
            }
        }
        function.begin()->eraseFromParent();
    }
}

void Modify::erase_test(Function& function) {
    /*
    set<Function*> constructors = collect_constructors(*(function.getParent()));
    cerr << "Constructor::(" << string(function.getParent()->getSourceFileName()) << "\n";
    for(auto e : constructors) {
        e->dump();
        cerr << "****\n";
    }


    set<Function*> deleteds = erase_test_internal(function, constructors);
    for(auto deleted : deleteds) {
        cerr << "[I] erase::\n";
//        deleted->dump();
        remove_function(*deleted);
        cerr << "****\n";
    }
    */

    cerr << "[I] erase:: " << string(function.getName()) << "\n";
    remove_function(function);

    /*
    set<Value*> users = last_user(function, {}, function);
    for(auto user : users) {
        cerr << "USER::\n";
//        user->dump();
        cerr << "====JUNE====\n";
    }

    assert(false);
    */
}

size_t Modify::erase_assert(Function& function) {
    if(!function.getFunctionType()->getReturnType()->isVoidTy()) {
        return 0;
    }

/*
    if(string(function.getName()) != "test_no_overflow_long_body") {
        return 0;
    }
*/

    vector<Instruction*> deletes;
    unsigned ret = 0;

    for(BasicBlock &b : function) {
        for(Instruction &i : b) {
            if(isa<CallInst>(&i)) {
                CallInst* ci = dyn_cast<CallInst>(&i);
                Function* cf = ci->getCalledFunction();
                if(cf != nullptr && 
                    ((cf->getName() == "__assert_fail") ||
                     (cf->getName() == "abort"))) {
                    if(find(deletes.begin(), deletes.end(), &i) == deletes.end()) {
                        deletes.push_back(&i);
                    }
                }
            }
        }
    }

/*
    cerr << "BEFORE=\n";
    function.dump();
    cerr << "=====\n";
*/

    for(auto e : deletes) {
        Instruction* i = ReturnInst::Create(function.getContext(), nullptr, e);
        e->eraseFromParent();
        i = i->getNextNode();
        if(i != nullptr && isa<UnreachableInst>(i)) {
            i->eraseFromParent();
        }
    }

/*
    cerr << "AFTER=\n";
    function.dump();
    cerr << "=====\n";\
*/

    return ret;
}

set<Function*> Modify::erase_test_internal(Value& value, const set<Function*> constructors) {

    set<Function*> ret;

    cerr << "Trace :\n";
//    value.dump();
    cerr << "=====\n";

    if(isa<Instruction>(&value)) {
        cerr << "Instruction!!!\n";
        BasicBlock* B = dyn_cast<Instruction>(&value)->getParent();
        if(B != nullptr) {
            Function* function = dyn_cast<Instruction>(&value)->getParent()->getParent();
            if(find(constructors.begin(), constructors.end(), function) != constructors.end()) {
                ret.insert(function);
                return ret;
            }
            for(auto e : erase_test_internal(*function, constructors)) {
                ret.insert(function);
            }
        }
        else {
            cerr << "BasicBlock is Null!!\n";
        }
    }

    for(auto user = value.user_begin(); user != value.user_end(); ++user) {
        for(auto e : erase_test_internal(**user, constructors)) {
            ret.insert(e);
        }
    }

    return ret;
}

set<Value*> Modify::last_user(Value& value, set<Function*> visited, const Function& target) {
    set<Value*> ret;

    cerr << "value :\n";
//    value.dump();

    if(!isa<Instruction>(&value)) {
        for(auto user = value.user_begin(); user != value.user_end(); ++user) {
            for(auto e : last_user(**user, visited, target)) {
                ret.insert(e);
            }
        }
        return ret;
    }

    Instruction* i = dyn_cast<Instruction>(&value);
    Function* called = get_called_function(*i);
    if(called != nullptr && called == &target) {
        ret.insert(i);
        return ret;
    }

    Function* function = i->getParent()->getParent();
    if(find(visited.begin(), visited.end(), function) != visited.end()) {
        return ret;
    }
    visited.insert(function);

    for(auto e : last_user(*function, visited, target)) {
        ret.insert(e);
    }

    return ret;
}

void Modify::remove_with_all_users(Value* value) {
    cerr << "[CURRENT] : ";
//    value->dump();
    cerr << "***\n";
    if(isa<Instruction>(value)) {
        cerr << "\t[MOTHER] : ";
//        dyn_cast<Instruction>(value)->getParent()->getParent()->dump();
        cerr << "***\n";
    }

    for(auto user = value->user_begin(); user != value->user_end(); user++) {
        cerr << "\t[USER] : ";
//        user->dump();
        cerr << "***\n";
    }

    for(auto user = value->user_begin(); user != value->user_end(); user++) {
        remove_with_all_users(*user);
    }
}

void Modify::generate_interface(Function& function) {
    BasicBlock& entry1 = function.getEntryBlock();
    Instruction& inst = *(entry1.getFirstInsertionPt());
    BasicBlock* link = entry1.splitBasicBlock(&inst);
    entry1.begin()->eraseFromParent();
    IRBuilder<> builder(&entry1);
    LLVMContext& ctx = function.getContext();
    Module& module = *function.getParent();

    GlobalVariable& glob_buf = *(get_global_buffer(module, false));
    GlobalVariable& glob_size = *(get_global_size(module, false));

    BasicBlock* entry2 = BasicBlock::Create(ctx, "", &function);
    BasicBlock* entry3 = BasicBlock::Create(ctx, "", &function);
    BasicBlock* entry4 = BasicBlock::Create(ctx, "", &function);
    BasicBlock* entry5 = BasicBlock::Create(ctx, "", &function);

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

//    glob_buf.dump();
//    glob_size.dump();
}

void Modify::generate_input(Instruction& I) {
    IRBuilder<> builder(&I);

    cerr << "Generate Input in function : " << string(I.getFunction()->getName()) << "\n";

    GlobalVariable* gv_b = get_global_buffer(*I.getModule(), true);
    GlobalVariable* gv_s = get_global_size(*I.getModule(), true);

    /*
    Value* buf = builder.CreateAlloca(builder.getInt8PtrTy());
    Value* n0 = builder.CreateLoad(gv_s);
    Value* add = builder.CreateAdd(n0, builder.getInt32(1));
    Value* call = builder.CreateCall(get_calloc_function(*(I.getModule())),
        {add, builder.getInt32(1)});
    builder.CreateStore(call, buf);
    Value* n1 = builder.CreateLoad(gv_b);
    Value* n2 = builder.CreateLoad(buf);
    Value* n3 = builder.CreateLoad(gv_s);
    builder.CreateCall(get_memcpy_function(*(I.getModule())),
        { n2, n1, n3, builder.getInt32(1), builder.getInt1(0) });
    Value* n4 = builder.CreateLoad(buf);
    Value* n5 = builder.CreateLoad(gv_s);
    set_argument(I, *n4, Config::get()->get_fuzz() - 1);
    if(Config::get()->get_size() != 0) {
        set_argument(I, *n5, Config::get()->get_size() - 1);
    }

    Instruction* fake = ReturnInst::Create(I.getContext());
    fake->insertAfter(&I);
    builder.SetInsertPoint(fake);

    Value* n6 = builder.CreateLoad(buf);
    builder.CreateCall(get_free_function(*(I.getModule())), { n6 });

    fake->eraseFromParent();
    */

    Value* n0 = builder.CreateLoad(gv_b);
    set_argument(I, *n0, Config::get()->get_fuzz() - 1);
    if(Config::get()->get_size() != 0) {
        Value* n1 = builder.CreateLoad(gv_s);
        set_argument(I, *n1, Config::get()->get_size() - 1);
    }
}
