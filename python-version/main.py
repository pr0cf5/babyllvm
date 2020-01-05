#!/usr/bin/python3

from ctypes import CFUNCTYPE, c_long, c_int, POINTER, cast, CDLL, addressof
import sys
import llvmlite.ir as llvmIR
import llvmlite.binding as llvm
import traceback

i8 = llvmIR.IntType(8)
i32 = llvmIR.IntType(32)
i64 = llvmIR.IntType(64)

i8_ptr = llvmIR.PointerType(i8)
i32_ptr = llvmIR.PointerType(i32)
i64_ptr = llvmIR.PointerType(i64)

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

	if type(x) == type((1,2)):
		if x[1] == None:
			return resolveRight(x[0])
		else:
			return resolveRight(x[1])
	else:
		assert(x != None and type(x) != type((1,2)))
		return x

def resolveLeft(x):

	if type(x) == tuple:
		if x[0] == type((1,2)):
			return resolveLeft(x[1])
		else:
			return resolveLeft(x[0])
	else:
		assert(x != None and type(x) != type((1,2)))
		return x

class bfProgram:
	def __init__ (self, c):
		self.code = c
		self.head = None
		self.br1 = None
		self.br2 = None
		self.isLinear = True

		pars = self.match_par()

		branchpos = self.code.find("[")

		if branchpos != -1:
			assert(branchpos in pars.keys())
			matchpos = pars[branchpos]
			self.head = bfProgram(self.code[:branchpos])
			self.br1 = bfProgram(self.code[branchpos+1:matchpos])
			self.br2 = bfProgram(self.code[matchpos+1:])
			self.isLinear = False

	def codegen(self, module):
		main_routine = findFunctionByName(module, "main_routine")

		if (self.isLinear == True):
			block = main_routine.append_basic_block()
			builder = llvmIR.IRBuilder()
			builder.position_at_end(block)

			for x in self.code:
				if x == '>':
					dptr = findGlobvarByName(module, "data_ptr")
					ori = builder.ptrtoint(builder.load(dptr), i64)
					one = llvmIR.Constant(i64, 1)
					inc = builder.inttoptr(builder.add(ori, one), i8_ptr)
					builder.store(inc, dptr)
				elif x == '<':
					dptr = findGlobvarByName(module, "data_ptr")
					ori = builder.ptrtoint(builder.load(dptr), i64)
					one = llvmIR.Constant(i64, 1)
					inc = builder.inttoptr(builder.sub(ori, one), i8_ptr)
					builder.store(inc, dptr)
				elif x == '+':
					dptr_ptr = findGlobvarByName(module, "data_ptr")
					dptr = builder.load(dptr_ptr)
					ori = builder.load(dptr)
					one = llvmIR.Constant(i8, 1)
					inc = builder.store(builder.add(ori, one), dptr)
				elif x == '-':
					dptr_ptr = findGlobvarByName(module, "data_ptr")
					dptr = builder.load(dptr_ptr)
					ori = builder.load(dptr)
					one = llvmIR.Constant(i8, 1)
					inc = builder.store(builder.sub(ori, one), dptr)
				elif x == '.':
					putchar = findFunctionByName(module, "putchar")
					dptr_ptr = findGlobvarByName(module, "data_ptr")
					dptr = builder.load(dptr_ptr)
					ori = builder.load(dptr)
					builder.call(putchar, [ori])

				elif x == ',':
					getchar = findFunctionByName(module, "getchar")
					dptr_ptr = findGlobvarByName(module, "data_ptr")
					dptr = builder.load(dptr_ptr)
					ch = builder.call(getchar, [])
					ori = builder.store(ch, dptr)

				else:
					raise Exception("invalid opcode %02x"%ord(x))

			return (block, None)

		else:
			# create all blocks
			headb = self.head.codegen(module)
			br1b = self.br1.codegen(module)
			br2b = self.br2.codegen(module)

			# emit code for head
			builder = llvmIR.IRBuilder()
			builder.position_at_end(resolveRight(headb))
			currentval = builder.load(builder.load(findGlobvarByName(module, "data_ptr")))
			zero = llvmIR.Constant(i8, 0)
			cond = builder.icmp_unsigned("==", currentval, zero)
			builder.cbranch(cond, resolveLeft(br2b), resolveLeft(br1b))

			# emit code for taken
			builder.position_at_end(resolveRight(br1b))
			currentval = builder.load(builder.load(findGlobvarByName(module, "data_ptr")))
			zero = llvmIR.Constant(i8, 0)
			cond = builder.icmp_unsigned("!=", currentval, zero)
			builder.cbranch(cond, resolveLeft(br1b), resolveLeft(br2b))

			return (headb, br2b)

	def match_par (self):
		# validate matching of [ and ]
		charset = "><+-.,"
		stk = []
		rv = dict()

		for i, x in enumerate(self.code):
			if x == '[':
				stk.append((x, i))
			elif x == ']':
				if (len(stk) == 0):
					raise Exception("unmatching parantheses in program")
				ch, idx = stk.pop()
				rv[idx] = i
		
		return rv

def compile(program, verbose=False):

	# initialize module and main_routine
	fty = llvmIR.FunctionType(i32, [i32])
	module = llvmIR.Module()
	main_routine = llvmIR.Function(module, fty, "main_routine")
	
	# add external functions
	fty = llvmIR.FunctionType(i32, [i8])
	putchar = llvmIR.Function(module, fty, "putchar")
	fty = llvmIR.FunctionType(i8, [])
	getchar = llvmIR.Function(module, fty, "getchar")

	# initialize data_ptr
	data_ptr = llvmIR.GlobalVariable(module, i8_ptr, "data_ptr")

	# initialize intro block
	intro = main_routine.append_basic_block()
	builder = llvmIR.IRBuilder()
	builder.position_at_end(intro)
	heap = libc.calloc(1, 0x3000)
	builder.store(llvmIR.Constant(i64, heap).inttoptr(i8_ptr), data_ptr)
	
	# compile bf code
	body = program.codegen(module)

	# append epilogue
	epilogue = main_routine.append_basic_block()
	builder.position_at_end(epilogue)
	builder.ret(llvmIR.Constant(i32, 1))

	# connect control flow
	builder.position_at_end(intro)
	builder.branch(resolveLeft(body))

	builder.position_at_end(resolveRight(body))
	builder.branch(epilogue)

	# verify generated IR
	strmod = str(module)
	llmod = llvm.parse_assembly(strmod)
	llmod.verify()

	if verbose:
		print(llmod)

	return llmod

def execute(llmod, verbose=False):
	target_machine = llvm.Target.from_default_triple().create_target_machine()

	with llvm.create_mcjit_compiler(llmod, target_machine) as ee:
		ee.add_global_mapping(llmod.get_global_variable("data_ptr")._ptr, libc.malloc(8))
		ee.finalize_object()
		cfptr = ee.get_function_address("main_routine")
		if verbose:
			print(target_machine.emit_assembly(llmod))
		cfunc = CFUNCTYPE(c_int, c_int)(cfptr)
		if cfunc(1) != 1:
			raise Exception("jitted-code returned an abnormal value")


if __name__ == "__main__":

	# initialize llvm backend
	llvm.initialize()
	llvm.initialize_native_target()
	llvm.initialize_native_asmprinter()
	llvm.load_library_permanently("/lib/x86_64-linux-gnu/libc.so.6")

	# initialize libc
	libc = CDLL("/lib/x86_64-linux-gnu/libc.so.6")
	c_long_p = POINTER(c_long)
	putchar = cast(addressof(libc.putchar), c_long_p).contents.value

	# print hello world
	example_prog = "++++++++[>++++[>++>+++>+++>+<<<<-]>+>+>->>+[<]<-]>>.>---.+++++++..+++.>>.<-.<.+++.------.--------.>>+.>++."
	execute(compile(bfProgram(example_prog)))

	while True:
		code = input(">>> ")
		pg = bfProgram(code)
		llmod = compile(pg)
		execute(llmod)
