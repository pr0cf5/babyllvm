#include <iostream>
#include "ast.hpp"

int main(int argc, char **argv) {
    std::unique_ptr<Expr> one(new NumExpr(1));
    std::unique_ptr<Expr> two(new NumExpr(2));
    std::unique_ptr<Expr> fin(new AddExpr(std::move(one), std::move(two)));
    printf("ast: %s\n", fin->to_string().c_str());
    return 0;
}
