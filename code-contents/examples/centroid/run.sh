#!/bin/sh

make -B TARGET=centroid_clang.out
make -B CC=gcc LD=gcc TARGET=centroid_gcc.out

echo "clang"
taskset -c 0 ./centroid_clang.out
echo "gcc"
taskset -c 0 ./centroid_gcc.out

for f in $(find . -name "centroid*.dat" | sort); do diff -s -q exp.dat $f; done;
