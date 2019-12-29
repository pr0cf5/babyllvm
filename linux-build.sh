#!/bin/sh
aclocal
automake --add-missing
autoconf
CXX=clang++ CXXFLAGS="-g -O3 $(llvm-config --cxxflags) $(llvm-config --ldflags) $(llvm-config  --system-libs) $(llvm-config --libs core)" ./configure
make
