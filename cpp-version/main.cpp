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
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/Support/TargetSelect.h"
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <vector>
#include <memory>

#include "parse.hpp"
#include "codegen.hpp"

static uint8_t *DPTR;

static void printmenu() {
    puts("[1] add program\n[2] compile and execute program");
    printf("Your choice: ");
}

static int read_int() {
    char buf[0x20];
    fgets(buf, sizeof(buf), stdin);
    return atoi(buf);
}

static void initialize() {
    setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();
}

int main(int argc, char **argv) {
    std::unique_ptr<bfProgram> program;
    size_t code_len;
    uint8_t *code;
    char *pos;
    
    initialize();
    code = (uint8_t *)malloc(0x2000);
    
    while(1) {
        printmenu();
        switch (read_int()) {
            /* add program */
            case 1: {
                fgets((char *)code, 0x2000, stdin);
                if ((pos = strchr((char *)code, '\n'))) {
                    *pos = '\0';
                }
                code_len = strlen((const char *)code);
                program = std::unique_ptr<bfProgram> (new bfProgram(code, code_len));
                /* don't free code, it is maintained for later use */
                
                if (program->parse_success) {
                    puts("[+] Your program is good to compile.");
                }
                else {
                    puts("[-] Your program has an error.");
                    program = std::unique_ptr<bfProgram> (nullptr);
                }
                break;
            }
            
            case 2: {
                puts("[*] compiling your program to LLVM IR");
                std::unique_ptr<llvm::Module> mod = compileToLLVMIR(std::move(program));
                puts("[+] compile done, dumping LLVM IR");
                llvm::Function *main_routine = mod->getFunction("main_routine");
                llvm::GlobalValue *data_ptr = mod->getNamedGlobal("data_ptr");
                main_routine->print(llvm::errs());
                puts("[*] now executing the compiled program");
                std::string err_str;
                llvm::ExecutionEngine *engine = llvm::EngineBuilder(std::move(mod)).setErrorStr(&err_str).create();
                if (!engine) {
                    printf("[!] error while creating engine: %s\n", err_str.c_str());
                    return 1;
                }
         	void *fptr = engine->getPointerToNamedFunction("main_routine");
                printf("[*] execution complete, jit@addr %p\n", fptr);
                puts("[*] execution complete");
                break;
            }
        }
    }
    return 0;
}
