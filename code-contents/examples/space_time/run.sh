#!/bin/sh

make -B TARGET=test_clang.out
make -B CC=gcc LD=gcc TARGET=test_gcc.out

for f in $(ls *.out | sort); do echo "$f: "; ./$f; echo ""; done;
