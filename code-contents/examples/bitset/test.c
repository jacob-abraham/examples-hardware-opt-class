#include "timing.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

char setbit_calc(char n, char b, char k) {
    n = (n & ~(1 << k)) | (b << k);
    return n;
}
char setbit_cond(char n, char b, char k) {
    n = b ? n | (1 << k) : n & ~(1 << k);
    return n;
}

void* bitset_bench(void* arg) {
    char (*func)(char, char, char) = (char (*)(char, char, char))arg;
    for(int n = 0; n < 256/*2^8*/; n++) {
        for(int k = 0; k < 8; k++) {
            func(n, 1, k);
            func(n, 0, k);
        }
    }
    return NULL;
}

int main(__attribute((unused)) int argc, __attribute((unused)) char** argv) {

    printf("setbit_calc: ");
    benchmark(bitset_bench, (void*)setbit_calc, 50, 10000, 0b01, NULL);
    printf("setbit_cond: ");
    benchmark(bitset_bench, (void*)setbit_cond, 50, 10000, 0b01, NULL);

    // accuracy check
    char n, k, s, c;
    printf("\nAccuracy\n");
    n = 0b101; k = 2;
    s = setbit_calc(n, 1, k);
    c = setbit_calc(n, 0, k);
    printf("setbit_calc: n=%-2hhx k=%-2hhu s=0x%02hhx c=0x%02hhx\n", n, k, s, c);
    n = 0b101; k = 2;
    s = setbit_cond(n, 1, k);
    c = setbit_cond(n, 0, k);
    printf("setbit_cond: n=%-2hhx k=%-2hhu s=0x%02hhx c=0x%02hhx\n", n, k, s, c);
}
