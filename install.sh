#!/bin/sh
autoconf
automake
CXX=clang++ CXXFLAGS="-g -O3 $(llvm-config --cxxflags) $(llvm-config --ldflags) $(llvm-config  --system-libs) $(llvm-config --libs core)" ./configure
make
