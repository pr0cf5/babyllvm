#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "parse.hpp"

static llvm::LLVMContext ctx;
unsigned long blockID = 0;

llvm::BasicBlock *bfProgram::codegen(llvm::Module *mod, llvm::Function *func) {
    char block_name[0x20];
    
    if (is_branch) {
        snprintf(block_name, sizeof(block_name) - 1, "block%lu", blockID);
        blockID++;
        llvm::BasicBlock *bb = llvm::BasicBlock::Create(ctx, block_name, func);
        llvm::BasicBlock *tknb = taken->codegen(mod, func);
        llvm::BasicBlock *ntknb = notTaken->codegen(mod, func);
        llvm::IRBuilder<> builder(bb);
        llvm::Value *data_ptr_ptr = mod->getNamedValue("data_ptr");
        llvm::LoadInst *data_ptr = builder.CreateLoad(data_ptr_ptr);
        llvm::LoadInst *dd = builder.CreateLoad(data_ptr);
        llvm::Constant *zero = builder.getInt8(0);
        llvm::Value *dataIsZero = builder.CreateICmpEQ(dd, zero, "tmp");
        builder.CreateCondBr(dataIsZero, tknb, ntknb);
        return bb;
    }
    else {
        snprintf(block_name, sizeof(block_name) - 1, "block%lu", blockID);
        blockID++;
        llvm::BasicBlock *bb = llvm::BasicBlock::Create(ctx, block_name, func);
        llvm::IRBuilder<> builder(bb);
        
        /* now compile the code */
        for (off_t i = 0; i < _code_len; i++) {
            switch (_code[i]) {
                case '>': {
                    llvm::Value *data_ptr_ptr = mod->getNamedValue("data_ptr");
                    llvm::LoadInst *data_ptr = builder.CreateLoad(data_ptr_ptr);
                    llvm::Value *inc = builder.CreateAdd(data_ptr, builder.getInt64(1));
                    builder.CreateStore(inc, data_ptr_ptr);
                    break;
                }
                case '<': {
                    llvm::Value *data_ptr_ptr = mod->getNamedValue("data_ptr");
                    llvm::LoadInst *data_ptr = builder.CreateLoad(data_ptr_ptr);
                    llvm::Value *dec = builder.CreateSub(data_ptr, builder.getInt64(1));
                    builder.CreateStore(dec, data_ptr_ptr);
                    break;
                }
                case '+': {
                    llvm::Value *data_ptr_ptr = mod->getNamedValue("data_ptr");
                    llvm::LoadInst *data_ptr = builder.CreateLoad(data_ptr_ptr);
                    llvm::LoadInst *ori = builder.CreateLoad(data_ptr);
                    llvm::Value *inc = builder.CreateAdd(builder.getInt8(1), ori);
                    builder.CreateStore(inc, data_ptr);
                    break;
                }
                case '-': {
                    llvm::Value *data_ptr_ptr = mod->getNamedValue("data_ptr");
                    llvm::LoadInst *data_ptr = builder.CreateLoad(data_ptr_ptr);
                    llvm::LoadInst *ori = builder.CreateLoad(data_ptr);
                    llvm::Value *dec = builder.CreateSub(ori, builder.getInt8(1));
                    builder.CreateStore(dec, data_ptr);
                    break;
                }
                default: {
                    fprintf(stderr,"invalid opcode: %02x\n", _code[i]);
                    exit(-1);
                }
            }
        }
        return bb;
    }
}

void compileToLLVMIR(std::unique_ptr<bfProgram> prog) {
    
    /* initialize module and main routine function */
    printf("go1\n");
    llvm::Module *mod = new llvm::Module("main", ctx);
    mod->getOrInsertFunction("main_routine", llvm::IntegerType::get(ctx, 32));
    llvm::Function *main_routine = mod->getFunction("main_routine");
    main_routine->setCallingConv(llvm::CallingConv::C);

    /* initialize data pointer */
    mod->getOrInsertGlobal("data_ptr", llvm::PointerType::get(llvm::IntegerType::get(ctx, 8), 64));
    
    /* compile */
    llvm::BasicBlock *block = prog->codegen(mod, main_routine);
    main_routine->print(llvm::errs());
}
