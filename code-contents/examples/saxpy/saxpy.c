
#include <immintrin.h>
#include "timing.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <unistd.h>

struct test_args {
    size_t n;
    float* a;
    float* x;
    float* y;
    float* r;
};
#define UNPACK_ARGS(ARGS)                                                      \
    size_t n = ((struct test_args*)ARGS)->n;                                   \
    float* a = ((struct test_args*)ARGS)->a;                                   \
    float* x = ((struct test_args*)ARGS)->x;                                   \
    float* y = ((struct test_args*)ARGS)->y;                                   \
    float* r = ((struct test_args*)ARGS)->r;

__attribute__((always_inline)) void saxpy_asm(float* a, float* x, float* y,
                                              float* r, size_t n);
__attribute__((always_inline)) void saxpy_bad_asm(float* a, float* x, float* y,
                                                  float* r, size_t n);
void* saxpy_asm_unwrapped(void* arg) {
    UNPACK_ARGS(arg)
    saxpy_asm(a, x, y, r, n);
    return NULL;
}
void* saxpy_bad_asm_unwrapped(void* arg) {
    UNPACK_ARGS(arg)
    saxpy_bad_asm(a, x, y, r, n);
    return NULL;
}

void* saxpy(void* arg) {
    UNPACK_ARGS(arg)
    float scalar = *a;
    for(size_t i = 0; i < n; i++) {
        r[i] = scalar * x[i] + y[i];
    }
    return NULL;
}

#ifdef HAS_SSE
__attribute__((always_inline)) void saxpy_asm_sse(float* a, float* x, float* y,
                                                  float* r, size_t n);
void* saxpy_asm_sse_unwrapped(void* arg) {
    UNPACK_ARGS(arg)
    saxpy_asm_sse(a, x, y, r, n);
    return NULL;
}

void* saxpy_sse(void* arg) {
    UNPACK_ARGS(arg)
    __m128 scalar = _mm_load_ps1(a);
    __m128 ld1, ld2, res;
    for(size_t i = 0; i < n; i += 4) {
        ld1 = _mm_load_ps(x + i);
        ld2 = _mm_load_ps(y + i);
        res = _mm_mul_ps(scalar, ld1);
        res = _mm_add_ps(res, ld2);
        _mm_store_ps(r + i, res);
    }
    return NULL;
}
#endif

#ifdef HAS_FMA
void* saxpy_fma(void* arg) {
    UNPACK_ARGS(arg)
    __m128 scalar = _mm_load_ps1(a);
    __m128 ld1, ld2, res;
    for(size_t i = 0; i < n; i += 4) {
        ld1 = _mm_load_ps(x + i);
        ld2 = _mm_load_ps(y + i);
        res = _mm_fmadd_ps(scalar, ld1, ld2);
        _mm_store_ps(r + i, res);
    }
    return NULL;
}
#endif

#ifdef HAS_AVX
void* saxpy_avx(void* arg) {
    UNPACK_ARGS(arg)
    __m256 scalar = _mm256_broadcast_ss(a);
    __m256 ld1, ld2, res;
    for(size_t i = 0; i < n; i += 8) {
        ld1 = _mm256_load_ps(x + i);
        ld2 = _mm256_load_ps(y + i);
        res = _mm256_mul_ps(scalar, ld1);
        res = _mm256_add_ps(res, ld2);
        _mm256_store_ps(r + i, res);
    }
    return NULL;
}
#endif

#if defined(HAS_FMA) && defined(HAS_AVX)
void* saxpy_fma256(void* arg) {
    UNPACK_ARGS(arg)
    __m256 scalar = _mm256_broadcast_ss(a);
    __m256 ld1, ld2, res;
    for(size_t i = 0; i < n; i += 8) {
        ld1 = _mm256_load_ps(x + i);
        ld2 = _mm256_load_ps(y + i);
        res = _mm256_fmadd_ps(scalar, ld1, ld2);
        _mm256_store_ps(r + i, res);
    }
    return NULL;
}
#endif

int main() {

    size_t n = 1024 * 4; // each array is a 1/4 page
    float* a = (float*)aligned_alloc(64, sizeof(float));
    *a = 1 + rand() / (1 + rand() % 300);

    // cannot be freed
    float* x = (float*)aligned_alloc(64, n * sizeof(float));
    float* y = (float*)aligned_alloc(64, n * sizeof(float));

    for(size_t i = 0; i < n; i++) {
        x[i] = 1 + rand() / (1 + rand() % 200);
        y[i] = 1 + rand() / (1 + rand() % 200);
    }

    float* r = (float*)aligned_alloc(64, n * sizeof(float));

    struct test_args arg = {.n = n, .a = a, .x = x, .y = y, .r = r};

    printf("Scalar:         ");
    benchmark(saxpy, (void*)(&arg), 50, 10000, 0b01, NULL);
    printf("Scalar Asm:     ");
    benchmark(saxpy_asm_unwrapped, (void*)(&arg), 50, 10000, 0b01, NULL);
    printf("Scalar Bad Asm: ");
    benchmark(saxpy_bad_asm_unwrapped, (void*)(&arg), 50, 10000, 0b01, NULL);
#ifdef HAS_SSE

    printf("SSE:            ");
    benchmark(saxpy_sse, (void*)(&arg), 50, 10000, 0b01, NULL);
    printf("SSE Asm:        ");
    benchmark(saxpy_asm_sse_unwrapped, (void*)(&arg), 50, 10000, 0b01, NULL);
#endif
#ifdef HAS_FMA
    printf("FMA:            ");
    benchmark(saxpy_fma, (void*)(&arg), 50, 10000, 0b01, NULL);
#endif
#ifdef HAS_AVX
    printf("AVX:            ");
    benchmark(saxpy_avx, (void*)(&arg), 50, 10000, 0b01, NULL);
#endif
#if defined(HAS_FMA) && defined(HAS_AVX)
    printf("FMA256:         ");
    benchmark(saxpy_fma256, (void*)(&arg), 50, 10000, 0b01, NULL);
#endif
}
