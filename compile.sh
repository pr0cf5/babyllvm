#!/bin/sh
clang++ -g -O3 `llvm-config --cxxflags --ldflags --system-libs --libs core` -o program main.cpp parse.cpp codegen.cpp
