
#include <immintrin.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "timing.h"

struct test_args {
    float* a;
    float* result;
};

#define iterations 10000

void* c_hadd(void* args) {
    float* a = ((struct test_args*)args)->a;
    //without volatile, it only runs the loop once. look at asm: checks if result is a member of array a, if it isnt, then we can execute only once
    //https://godbolt.org/z/MxdfrzjM1
    volatile float* result = ((struct test_args*)args)->result;
    #ifdef BENCHMARK
    for(size_t i = 0; i < iterations; i++)
    #endif
    *result = a[0] + a[1] + a[2] + a[3];
    return NULL;
}
void* hardware_hadd(void* args);
void* emulate_hadd_1(void* args);
void* emulate_hadd_2(void* args);

int main(__attribute((unused)) int argc, __attribute((unused)) char** argv) {

    // cannot be freed
    float* a = (float*)aligned_alloc(64, 4 * sizeof(float));
    float* result = (float*)aligned_alloc(64, sizeof(float));

    struct test_args arg = {.a = a, .result = result};

    a[0] = 17.0f;
    a[1] = 39.0f;
    a[2] = 13.0f;
    a[3] = 43.0f;

#ifdef BENCHMARK

    printf("C HADD         : ");
    benchmark(c_hadd, (void*)(&arg), 50, 1000, 0b01, NULL);
    printf("HARDWARE HADD  : ");
    benchmark(hardware_hadd, (void*)(&arg), 50, 1000, 0b01, NULL);
    printf("EMULATED HADD 1: ");
    benchmark(emulate_hadd_1, (void*)(&arg), 50, 1000, 0b01, NULL);
    printf("EMULATED HADD 2: ");
    benchmark(emulate_hadd_2, (void*)(&arg), 50, 1000, 0b01, NULL);

#else

    // accuracy check
    hardware_hadd((void*)(&arg));
    printf("HA : %5.2f+%5.2f+%5.2f+%5.2f=%5.2f\n", a[0], a[1], a[2], a[3],
           *result);
    emulate_hadd_1((void*)(&arg));
    printf("EM1: %5.2f+%5.2f+%5.2f+%5.2f=%5.2f\n", a[0], a[1], a[2], a[3],
           *result);
    emulate_hadd_2((void*)(&arg));
    printf("EM2: %5.2f+%5.2f+%5.2f+%5.2f=%5.2f\n", a[0], a[1], a[2], a[3],
           *result);

#endif
}
