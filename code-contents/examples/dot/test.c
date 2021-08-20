

#include <immintrin.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "timing.h"

struct test_args {
    float* a;
    float* b;
    float* result;
    size_t n;
};
#define UNPACK_ARGS(ARGS)                                                      \
    float* a = ((struct test_args*)ARGS)->a;                                   \
    float* b = ((struct test_args*)ARGS)->b;                                   \
    float* result = ((struct test_args*)ARGS)->result;                         \
    size_t n = ((struct test_args*)ARGS)->n;

//#pragma STDC FP_CONTRACT ON
void* scalar_dp(void* arg) {
    UNPACK_ARGS(arg)
    float sum = 0;
    for(size_t i = 0; i < n; i++) {
        sum += a[i] * b[i];
    }
    *result = sum;
    return NULL;
}
void* vector_dp(void* arg) {
    UNPACK_ARGS(arg)
    float sum1 = 0;
    float sum2 = 0;
    float sum3 = 0;
    float sum4 = 0;
    __m128 temp1, temp2, temp3, temp4;
    for(size_t i = 0; i < n; i += 16) {
        temp1 = _mm_dp_ps(_mm_load_ps(a + i), _mm_load_ps(b + i), 0b11110001);
        temp2 = _mm_dp_ps(_mm_load_ps(a + i + 4), _mm_load_ps(b + i + 4),
                          0b11110001);
        temp3 = _mm_dp_ps(_mm_load_ps(a + i + 8), _mm_load_ps(b + i + 8),
                          0b11110001);
        temp4 = _mm_dp_ps(_mm_load_ps(a + i + 12), _mm_load_ps(b + i + 12),
                          0b11110001);

        sum1 += _mm_cvtss_f32(temp1);
        sum2 += _mm_cvtss_f32(temp2);
        sum3 += _mm_cvtss_f32(temp3);
        sum4 += _mm_cvtss_f32(temp4);
    }
    *result = sum1 + sum2 + sum3 + sum4;
    return NULL;
}

void* vector_dp_fma(void* arg) {
    UNPACK_ARGS(arg)
    __m128 temp1, temp2, temp3, temp4;
    temp1 = temp2 = temp3 = temp4 = _mm_set_ps1(0);
    for(size_t i = 0; i < n; i += 16) {
        temp1 = _mm_fmadd_ps(_mm_load_ps(a + i), _mm_load_ps(b + i), temp1);
        temp2 =
            _mm_fmadd_ps(_mm_load_ps(a + i + 4), _mm_load_ps(b + i + 4), temp2);
        temp3 =
            _mm_fmadd_ps(_mm_load_ps(a + i + 8), _mm_load_ps(b + i + 8), temp3);
        temp4 = _mm_fmadd_ps(_mm_load_ps(a + i + 12), _mm_load_ps(b + i + 12),
                             temp4);
    }
    __m128 sum12 = _mm_add_ps(temp1, temp2);
    __m128 sum34 = _mm_add_ps(temp3, temp4);
    __m128 vsum = _mm_add_ps(sum12, sum34);

    // A  B  C  D
    // C  D  A  B
    // AC BD CA DB
    // DB CA BD AC
    __m128 shuf = _mm_shuffle_ps(vsum, vsum, _MM_SHUFFLE(1, 0, 3, 2));
    vsum = _mm_add_ps(vsum, shuf);
    shuf = _mm_shuffle_ps(vsum, vsum, _MM_SHUFFLE(0, 1, 2, 3));
    vsum = _mm_add_ss(vsum, shuf);

    *result = _mm_cvtss_f32(vsum);
    return NULL;
}

#define rand_range_int(low, high) low + rand() % (high + 1 - low)
#define rand_range_float(low, high) (float)(rand_range_int(low, high))

int main(__attribute((unused)) int argc, __attribute((unused)) char** argv) {

    srand(time(NULL));

    size_t n = 1024 * 4; // each array is a 1/4 page

    // float *result = (float *)aligned_alloc(64, sizeof(float));
    float result;
    float* a = (float*)aligned_alloc(64, n * sizeof(float));
    float* b = (float*)aligned_alloc(64, n * sizeof(float));

    struct test_args arg = {.a = a, .b = b, .result = &result, .n = n};

    for(size_t i = 0; i < n; i++) {
        a[i] = rand_range_float(1, 200);
        b[i] = rand_range_float(1, 200);
    }
    /*for(size_t i = 0; i < n; i++) {
        a[i] = b[i] = i+1.0f;
    }*/

    printf("Scalar DP    : ");
    benchmark(scalar_dp, (void*)(&arg), 50, 5000, 0b01, NULL);
    printf("Vector DP    : ");
    benchmark(vector_dp, (void*)(&arg), 50, 5000, 0b01, NULL);
    printf("Vector DP FMA: ");
    benchmark(vector_dp_fma, (void*)(&arg), 50, 5000, 0b01, NULL);

    // accuracy check
    printf("\nAccuracy\n");
    scalar_dp((void*)(&arg));
    float s_dp = *(arg.result);
    printf("S   : vsum=%16.6f\n", s_dp);
    vector_dp((void*)(&arg));
    float v_dp = *(arg.result);
    printf("VDP : vsum=%16.6f\n", v_dp);
    vector_dp_fma((void*)(&arg));
    float v_dp_fma = *(arg.result);
    printf("VFMA: vsum=%16.6f\n", v_dp_fma);
}
