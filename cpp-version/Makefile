CXX=clang++
CXXFLAGS=-g -O3 `llvm-config --cxxflags --ldflags --system-libs --libs all`

chal: main.o parse.o codegen.o
	$(CXX) -o chal $(CXXFLAGS) parse.o main.o codegen.o 
clean:
	rm -f ./chal main.o parse.o codegen.o
