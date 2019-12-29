class Expr {
public:
    virtual std::string to_string(void);
};

class NumExpr : public Expr {
public:
    int value;
    NumExpr(int n);
    std::string to_string(void);
};

class AddExpr : public Expr {
public:
    std::unique_ptr<Expr> Left, Right;
    AddExpr(std::unique_ptr<Expr> left, std::unique_ptr<Expr> right);
    std::string to_string(void);
};

