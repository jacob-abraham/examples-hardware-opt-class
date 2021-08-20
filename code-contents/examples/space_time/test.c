#include "timing.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

struct test_args {
    char* arr;
    size_t n_elements;
    size_t start;
    size_t stride;
};
#define UNPACK_ARGS(ARGS)                                                      \
    char* arr = ((struct test_args*)ARGS)->arr;                                \
    size_t n_elements = ((struct test_args*)ARGS)->n_elements;                 \
    size_t start = ((struct test_args*)ARGS)->start;                           \
    size_t stride = ((struct test_args*)ARGS)->stride;

void* unpacked_bool_bench_set(void* arg) {
    UNPACK_ARGS(arg)
    for(size_t i = start; i < n_elements; i += stride) {
        arr[i] = 1;
    }
    return NULL;
}
void* unpacked_bool_bench_clear(void* arg) {
    UNPACK_ARGS(arg)
    for(size_t i = start; i < n_elements; i += stride) {
        arr[i] = 0;
    }
    return NULL;
}
void* packed_bool_bench_set(void* arg) {
    UNPACK_ARGS(arg)
    for(size_t i = start; i < n_elements; i += stride) {
        unsigned bit_location = i & 0b111;
        unsigned byte_location = i >> 3;
        arr[byte_location] |= (1 << bit_location);
    }
    return NULL;
}
void* packed_bool_bench_clear(void* arg) {
    UNPACK_ARGS(arg)
    for(size_t i = start; i < n_elements; i += stride) {
        unsigned bit_location = i & 0b111;
        unsigned byte_location = i >> 3;
        arr[byte_location] &= ~(1 << bit_location);
    }
    return NULL;
}

extern void* packed_bool_bench_set_asm(void* arg);
extern void* packed_bool_bench_clear_asm(void* arg);

int main(__attribute((unused)) int argc, __attribute((unused)) char** argv) {

    size_t n = 10000000;
    char* arr = (char*)aligned_alloc(64, n * sizeof(char));
    struct test_args test_data =
    {.arr = arr,
     .n_elements = n,
     .start = 0,
     .stride = 1 };

#define TEST_SUITE(V)                                                          \
    V(unpacked_bool_bench_set)                                                 \
    V(unpacked_bool_bench_clear)                                               \
    V(packed_bool_bench_set)                                                   \
    V(packed_bool_bench_clear)                                                 \
    V(packed_bool_bench_set_asm)                                               \
    V(packed_bool_bench_clear_asm)

#define BENCH(func)                                                            \
    printf(#func ": ");                                                        \
    benchmark(func, (void*)(&test_data), 50, 10, 0b01, NULL);

    printf("start %ld stride %ld\n", test_data.start, test_data.stride);
    TEST_SUITE(BENCH)

    test_data.start = 7;
    test_data.stride = 3;
        printf("start %ld stride %ld\n", test_data.start, test_data.stride);
    TEST_SUITE(BENCH)
}
