#!/bin/sh
clang++ -g -O3 `llvm-config --cxxflags --ldflags --system-libs --libs all` -o program main.cpp parse.cpp codegen.cpp
