#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/CallingConv.h"
#include <string>
#include <vector>
#include <memory>
#include "ast.hpp"

using namespace llvm;

std::string Expr::to_string() {
    return std::string("expr");
}

NumExpr::NumExpr(int val) {
    value = val;
}

std::string NumExpr::to_string() {
    char buf[0x20];
    snprintf(buf, sizeof(buf) - 1, "%d", value);
    return std::string(buf);
}

AddExpr::AddExpr(std::unique_ptr<Expr> left, std::unique_ptr<Expr> right) {
    Left = std::move(left);
    Right = std::move(right);
}

std::string AddExpr::to_string() {
    return "("+ Left->to_string() + " + " + Right->to_string() +")";
}
