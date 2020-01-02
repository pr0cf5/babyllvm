#!/usr/bin/python3

from ctypes import CFUNCTYPE, c_int, POINTER
import sys
import llvmlite.ir as llvmIR
import llvmlite.binding as llvm
import traceback

def findFunctionByName(module, fname):
	for f in module.functions:
		if f._name == fname:
			return f
	return None

def findGlobvarByName(module, gname):
	for g in module.globals:
		if g == gname:
			return module.globals[g]
	return None

def resolveRight(x):
	if (type(x) == type((1,2))):
		return resolveRight(x[1])
	return x

def resolveLeft(x):
	if (type(x) == type((1,2))):
		return resolveLeft(x[0])
	return x

class bfProgram:
	def __init__ (self, c):
		self.code = c
		self.head = None
		self.br1 = None
		self.br2 = None
		self.isLinear = True

		if not self.validate_code():
			raise Exception("invalid bf code")

		branchpos = self.code.find("[")

		if branchpos != -1:
			matchpos = self.code.find("]")
			assert(matchpos != -1)
			self.head = bfProgram(self.code[:branchpos])
			self.br1 = bfProgram(self.code[branchpos + 1:matchpos])
			self.br2 = bfProgram(self.code[matchpos + 1:])
			self.isLinear = False

	def codegen(self, module):
		main_routine = findFunctionByName(module, "main_routine")

		if (self.isLinear == True):
			return (main_routine.append_basic_block(), None)

		else:
			# create all blocks
			headb = resolveLeft(self.head.codegen(module))
			br1b = resolveLeft(self.br1.codegen(module))
			br2b = resolveLeft(self.br2.codegen(module))

			# emit code for head
			builder = llvmIR.IRBuilder()
			builder.position_at_end(headb)
			currentval = builder.load(builder.load(findGlobvarByName(module, "data_ptr")))
			zero = llvmIR.Constant(llvmIR.IntType(8), 0)
			cond = builder.icmp_unsigned("==", currentval, zero)
			builder.cbranch(cond, br1b, br2b)

			# emit code for taken
			builder.position_at_end(br1b)
			currentval = builder.load(builder.load(findGlobvarByName(module, "data_ptr")))
			zero = llvmIR.Constant(llvmIR.IntType(8), 0)
			cond = builder.icmp_unsigned("!=", currentval, zero)
			builder.cbranch(cond, br1b, br2b)

			return (headb, br2b)

	def validate_code (self):
		# validate matching of [ and ]
		charset = "><+-.,"
		try:
			stk = []
			for x in self.code:
				if x == '[':
					stk.append(x)
				elif x == ']':
					if stk.pop() != '[':
						return False
				elif not (x in charset):
					return False
			return True

		except Exception as e:
			print(e)
			return False

def compile(program):

	# initialize main_routine
	fty = llvmIR.FunctionType(llvmIR.IntType(32), [llvmIR.IntType(32)])
	module = llvmIR.Module()
	main_routine = llvmIR.Function(module, fty, "main_routine")

	# initialize data_ptr
	gty = llvmIR.PointerType(llvmIR.IntType(8))
	data_ptr = llvmIR.GlobalVariable(module, gty, "data_ptr")

	# initialize intro block
	intro = main_routine.append_basic_block()
	builder = llvmIR.IRBuilder()
	builder.position_at_end(intro)
	builder.store(llvmIR.Constant(llvmIR.IntType(64), 0xc00000).inttoptr(gty), data_ptr)
	
	# compile bf code
	m1, m2 = program.codegen(module)


	# append epilogue
	epilogue = main_routine.append_basic_block()
	builder.position_at_end(epilogue)
	builder.ret(llvmIR.Constant(llvmIR.IntType(32), 1))

	# connect control flow
	builder.position_at_end(intro)
	builder.branch(m1)
	if m2 == None:
		builder.position_at_end(m1)
		builder.branch(epilogue)
	else:
		builder.position_at_end(resolveRight(m2))
		builder.branch(epilogue)

	# print IR
	strmod = str(module)
	print(strmod)
	llmod = llvm.parse_assembly(strmod)
	print(llmod)


if __name__ == "__main__":

	llvm.initialize()
	llvm.initialize_native_target()
	llvm.initialize_native_asmprinter()

	while True:
		code = input(">>> ")
		pg = bfProgram(code)
		compile(pg)
