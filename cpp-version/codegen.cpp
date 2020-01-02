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

static llvm::GlobalVariable *createGlob(llvm::Module *mod, const char *name, llvm::Type *ty) {
    mod->getOrInsertGlobal(name, ty);
    llvm::GlobalVariable *gVar = mod->getNamedGlobal(name);
    gVar->setLinkage(llvm::GlobalValue::CommonLinkage);
    return gVar;
}

llvm::BasicBlock *bfProgram::codegen(llvm::Module *mod) {
    char block_name[0x20];
    llvm::Function *func = mod->getFunction("main_routine");
    
    if (is_branch) {
        snprintf(block_name, sizeof(block_name) - 1, "block%lu", blockID);
        blockID++;
        llvm::BasicBlock *bb = llvm::BasicBlock::Create(ctx, block_name, func);
        llvm::BasicBlock *tknb = taken->codegen(mod);
        llvm::BasicBlock *ntknb = notTaken->codegen(mod);
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
		    llvm::Value *data_ptr_int = builder.CreatePtrToInt(data_ptr, llvm::IntegerType::get(ctx, 64));
                    llvm::Value *inc_int = builder.CreateAdd(data_ptr_int, builder.getInt64(1));
                    llvm::Value *inc = builder.CreateIntToPtr(inc_int, llvm::PointerType::getUnqual(llvm::IntegerType::get(ctx, 8)));
                    builder.CreateStore(inc, data_ptr_ptr);
                    break;
                }
                case '<': {
                    llvm::Value *data_ptr_ptr = mod->getNamedValue("data_ptr");
                    llvm::LoadInst *data_ptr = builder.CreateLoad(data_ptr_ptr);
		    llvm::Value *data_ptr_int = builder.CreatePtrToInt(data_ptr, llvm::IntegerType::get(ctx, 64));
                    llvm::Value *dec_int = builder.CreateSub(data_ptr_int, builder.getInt64(1));
                    llvm::Value *dec = builder.CreateIntToPtr(dec_int, llvm::PointerType::getUnqual(llvm::IntegerType::get(ctx, 8)));
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

std::unique_ptr<llvm::Module> compileToLLVMIR(std::unique_ptr<bfProgram> prog) {
    
    /* initialize module and main routine function */
    llvm::Module *mod = new llvm::Module("main", ctx);
    mod->getOrInsertFunction("main_routine", llvm::IntegerType::get(ctx, 32));
    llvm::Function *main_routine = mod->getFunction("main_routine");
    main_routine->setCallingConv(llvm::CallingConv::C);

    /* initialize data pointer */
    llvm::GlobalVariable *dptr = createGlob(mod, "data_ptr", llvm::PointerType::getUnqual(llvm::IntegerType::get(ctx, 8)));
    
    /* add first block to program which sets data ptr to a big enough heap pointer */
    llvm::BasicBlock *intro = llvm::BasicBlock::Create(ctx, "intro", main_routine);
    llvm::IRBuilder<> builder(intro);
    unsigned long heap_addr = (unsigned long)malloc(0x3000);
    llvm::Constant *addrInInt = llvm::ConstantInt::get(llvm::IntegerType::get(ctx, 64), heap_addr);
    llvm::Value *initial_dptr = llvm::ConstantExpr::getIntToPtr(addrInInt, llvm::PointerType::getUnqual(llvm::IntegerType::get(ctx, 8)));
    builder.CreateStore(initial_dptr, dptr);
    
    /* compile bf code */
    llvm::BasicBlock *block = prog->codegen(mod);
    builder.SetInsertPoint(intro);
    builder.CreateBr(block);

    /* add epilogue */
    llvm::BasicBlock *epilogue = llvm::BasicBlock::Create(ctx, "epilogue", main_routine);
    builder.SetInsertPoint(epilogue);
    builder.CreateRet(builder.getInt32(1));
    builder.SetInsertPoint(block);
    builder.CreateBr(epilogue);

    /* verify */
    if (llvm::verifyFunction(*main_routine, &llvm::errs())) {
	fprintf(stderr, "[!] compile error due to invalid IR\n");
	main_routine->print(llvm::errs());
	exit(-1);
    }

    return std::unique_ptr<llvm::Module>(mod);
}
