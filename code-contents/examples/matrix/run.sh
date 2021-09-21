#!/bin/sh

make -B

taskset -c 0 ./matrix.out
for f in $(find . -name "multiply*.dat" | sort); do diff -s -q C.dat $f; done;
