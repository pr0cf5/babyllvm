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
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <vector>
#include <memory>

#include "parse.hpp"
#include "codegen.hpp"

static void printmenu() {
    printf("[1] add program\n[2] compile program\n[3] execute program\n[4] show assembly\n");
    printf("Your choice: ");
}

static int read_int() {
    char buf[0x20];
    fgets(buf, sizeof(buf), stdin);
    return atoi(buf);
}

int main(int argc, char **argv) {
    std::unique_ptr<bfProgram> program;
    size_t code_len;
    uint8_t *code;
    while(1) {
        printmenu();
        switch (read_int()) {
            /* add program */
            case 1:
                printf("Enter length of program: ");
                code_len = read_int();
                if (code_len > 0x1000) {
                    fprintf(stderr, "code is too long!\n");
                }
                else {
                    uint8_t *code = (uint8_t *)calloc(1, code_len);
                    if (!code) {
                        fprintf(stderr, "out of memory!\n");
                        exit(-1);
                    }
                    fread(code, 1, code_len, stdin);
                    program = std::unique_ptr<bfProgram> (new bfProgram(code, code_len));
                    /* don't free code, it is maintained for later use */
                    
                    if (program->parse_success) {
                        puts("Your program is good to compile.");
                    }
                    else {
                        puts("Your program has an error.");
                        program = std::unique_ptr<bfProgram> (nullptr);
                    }
                }
                break;
            
            case 2:
                puts("compiling your program");
                compileToLLVMIR(std::move(program));
                break;
        }
    }
    return 0;
}
