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
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <vector>
#include <memory>

#include "parse.hpp"

static uint8_t *search_char (uint8_t *haystack, size_t length, uint8_t needle) {
    for (off_t i = 0; i < length; i++) {
        if (haystack[i] == needle) {
            return &haystack[i];
        }
    }
    return NULL;
}

static uint8_t *search_char_rev (uint8_t *haystack, size_t length, uint8_t needle) {
    for (off_t i = length - 1; i > -1; i--) {
        if (haystack[i] == needle) {
            return &haystack[i];
        }
    }
    return NULL;
}


bfProgram::bfProgram (uint8_t *code, size_t code_len) {
	/* parse the if then else statements */
    _code = code;
    _code_len = code_len;
    
    uint8_t *first = search_char(code, code_len, '[');
    if (!first) {
        is_branch = false;
        parse_success = true;
        /* validate if there is no matching ']' in the program */
        if (search_char(code, code_len, ']')) {
            fprintf(stderr, "compilation failure: invalid usage of [ and ]\n");
            parse_success = false;
        }
    }
    else {
        is_branch = true;
        parse_success = true;
        uint8_t *match = search_char_rev(code, code_len, ']');
        /* after [ */
        size_t tkn_length = match - first - 1;
        taken = std::unique_ptr<bfProgram>(new bfProgram(first + 1, tkn_length));
        /* after ] */
        size_t ntkn_length = code + code_len - match - 1;
        notTaken = std::unique_ptr<bfProgram>(new bfProgram(match + 1, ntkn_length));
    }
}

