#!/bin/sh

make -B TARGET=test_clang.out
make -B CC=gcc LD=gcc TARGET=test_gcc.out

make -B TARGET=test_clang_fm.out CFLAGS="-ffast-math"
make -B CC=gcc LD=gcc TARGET=test_gcc_fm.out CFLAGS="-ffast-math"

for f in $(ls *.out | sort); do echo "$f: "; ./$f; done;
