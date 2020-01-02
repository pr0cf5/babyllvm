#!/usr/bin/python3

from ctypes import CFUNCTYPE, c_int, POINTER
import sys
import llvmlite.ir as llvmIR
import llvmlite.binding as llvm

def compile():
	fty = llvmIR.FunctionType(llvmIR.IntType(32), [llvmIR.IntType(32)])
	module = llvmIR.Module()
	main_routine = llvmIR.Function(module, fty, name="main_routine")
	intro = main_routine.append_basic_block()

	builder = llvmIR.IRBuilder()
	builder.position_at_end(intro)
	rv = builder.add(llvmIR.Constant(llvmIR.IntType(32), 30), main_routine.args[0])
	builder.ret(rv)
	strmod = str(module)
	llmod = llvm.parse_assembly(strmod)
	print(llmod)


if __name__ == "__main__":

	llvm.initialize()
	llvm.initialize_native_target()
	llvm.initialize_native_asmprinter()

	while True:
		code = input(">>> ")
		compile()
