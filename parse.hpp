class bfProgram {
public:
    uint8_t *_code;
    size_t _code_len;
    std::unique_ptr<bfProgram> taken;
    std::unique_ptr<bfProgram> notTaken;
    bool is_branch;
    bool parse_success;
    bfProgram(uint8_t *code_, size_t code_len_);
    llvm::BasicBlock *codegen(llvm::Module *mod, llvm::Function *func);
};

