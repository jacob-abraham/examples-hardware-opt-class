
#include "timing.h"
#include <immintrin.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// we can have
// 1. Array of Structs
// 2. Struct of Arrays
// 3. Array of Struct of Arrays

typedef struct {
    int32_t x;
    int32_t y;
    int32_t z;
} Point_AoS, Point;

typedef struct {
    int32_t* x;
    int32_t* y;
    int32_t* z;
    size_t numPoints;
} Point_SoA;

#define AoSoA_TileSize 32
typedef struct {
    int32_t x[AoSoA_TileSize];
    int32_t y[AoSoA_TileSize];
    int32_t z[AoSoA_TileSize];
    size_t numPointsUsed;
} Point_AoSoA;

size_t loadPointsAoS(char* filename, Point_AoS** points) {
    FILE* fp = fopen(filename, "r");
    if(!fp)
        return 0;

    size_t numPoints;
    // read the first line, contains size information
    size_t ret = fscanf(fp, "%zu\n", &numPoints);
    if(ret != 1)
        return 0;
    *points = (Point_AoS*)aligned_alloc(64, sizeof(Point_AoS) * numPoints);

    for(size_t i = 0; i < numPoints; i++) {
        ret = fscanf(fp, "%d,%d,%d\n", &((*points)[i].x), &((*points)[i].y),
                     &((*points)[i].z));
        if(ret != 3)
            break;
    }
    fclose(fp);
    return numPoints;
}
Point_SoA* loadPointsSoA(char* filename) {
    FILE* fp = fopen(filename, "r");
    if(!fp)
        return 0;

    size_t numPoints;
    // read the first line, contains size information
    size_t ret = fscanf(fp, "%zu\n", &numPoints);
    if(ret != 1)
        return NULL;
    Point_SoA* points = (Point_SoA*)aligned_alloc(64, sizeof(Point_SoA));
    points->numPoints = numPoints;
    points->x = (int32_t*)aligned_alloc(64, sizeof(int32_t) * numPoints);
    points->y = (int32_t*)aligned_alloc(64, sizeof(int32_t) * numPoints);
    points->z = (int32_t*)aligned_alloc(64, sizeof(int32_t) * numPoints);

    for(size_t i = 0; i < numPoints; i++) {
        ret = fscanf(fp, "%d,%d,%d\n", points->x + i, points->y + i,
                     points->z + i);
        if(ret != 3)
            break;
    }
    fclose(fp);
    return points;
}
size_t loadPointsAoSoA(char* filename, Point_AoSoA** points) {
    FILE* fp = fopen(filename, "r");
    if(!fp)
        return 0;

    size_t numPoints;
    // read the first line, contains size information
    size_t ret = fscanf(fp, "%zu\n", &numPoints);
    if(ret != 1)
        return 0;
    size_t numStructs = (numPoints + (AoSoA_TileSize - 1)) / AoSoA_TileSize;
    *points = (Point_AoSoA*)aligned_alloc(64, sizeof(Point_AoSoA) * numStructs);

    for(size_t i = 0; i < numStructs; i++) {
        size_t j;
        for(j = 0; j < AoSoA_TileSize; j++) {
            ret = fscanf(fp, "%d,%d,%d\n", &((*points)[i].x[j]),
                         &((*points)[i].y[j]), &((*points)[i].z[j]));
            if(ret != 3)
                break;
        }
        (*points)[i].numPointsUsed = j;
    }
    fclose(fp);
    return numPoints;
}
void dumpCentroid(char* filename, Point* p) {
    FILE* fp = fopen(filename, "w");
    if(!fp)
        return;

    fprintf(fp, "%d,%d,%d\n", p->x, p->y, p->z);
    fclose(fp);
}

#define printVector256(_v)                                                     \
    {                                                                          \
        union {                                                                \
            __m256i v;                                                         \
            int32_t i[8];                                                      \
        } s;                                                                   \
        s.v = _v;                                                              \
        printf(#_v ": %d,%d,%d,%d,%d,%d,%d,%d\n", s.i[0], s.i[1], s.i[2],      \
               s.i[3], s.i[4], s.i[5], s.i[6], s.i[7]);                        \
    }
#define printVector128(_v)                                                     \
    {                                                                          \
        union {                                                                \
            __m128i v;                                                         \
            int32_t i[4];                                                      \
        } s;                                                                   \
        s.v = _v;                                                              \
        printf(#_v ": %d,%d,%d,%d\n", s.i[0], s.i[1], s.i[2], s.i[3]);         \
    }

typedef struct {
    Point_AoS* AoS;
    size_t numPoints;
    Point* centroid;
} test_args_AoS_t;
#define UNPACK_ARGS_AoS(ARGS)                                                  \
    Point_AoS* AoS = ((test_args_AoS_t*)ARGS)->AoS;                            \
    size_t numPoints = ((test_args_AoS_t*)ARGS)->numPoints;                    \
    Point* centroid = ((test_args_AoS_t*)ARGS)->centroid;

typedef struct {
    Point_SoA* SoA;
    Point* centroid;
} test_args_SoA_t;
#define UNPACK_ARGS_SoA(ARGS)                                                  \
    Point_SoA* SoA = ((test_args_SoA_t*)ARGS)->SoA;                            \
    Point* centroid = ((test_args_SoA_t*)ARGS)->centroid;

typedef struct {
    Point_AoSoA* AoSoA;
    size_t numPoints;
    Point* centroid;
} test_args_AoSoA_t;
#define UNPACK_ARGS_AoSoA(ARGS)                                                \
    Point_AoSoA* AoSoA = ((test_args_AoSoA_t*)ARGS)->AoSoA;                    \
    size_t numPoints = ((test_args_AoSoA_t*)ARGS)->numPoints;                  \
    Point* centroid = ((test_args_AoSoA_t*)ARGS)->centroid;

// some helpers
__attribute((always_inline)) int32_t hadd_epi32_128(__m128i x) {
    __m128i shuf = _mm_shuffle_epi32(x, 0b01001110);
    x = _mm_add_epi32(x, shuf);
    shuf = _mm_shuffle_epi32(x, 0b00011011);
    x = _mm_add_epi32(x, shuf);
    return (int32_t)_mm_cvtsi128_si32(x);
}
__attribute((always_inline)) int32_t hadd_epi32_256(__m256i x) {
    __m128i low = _mm256_castsi256_si128(x);
    __m128i high = _mm256_extracti128_si256(x, 1);
    __m128i added = _mm_add_epi32(low, high);
    return hadd_epi32_128(added);
}

void* centroid_AoS_c(void* arg) {
    UNPACK_ARGS_AoS(arg);

    int32_t sum_x = 0;
    int32_t sum_y = 0;
    int32_t sum_z = 0;
    for(size_t i = 0; i < numPoints; i++) {
        sum_x += AoS[i].x;
        sum_y += AoS[i].y;
        sum_z += AoS[i].z;
    }
    centroid->x = sum_x / (int32_t)numPoints;
    centroid->y = sum_y / (int32_t)numPoints;
    centroid->z = sum_z / (int32_t)numPoints;

    return NULL;
}

int32_t idx_x_data[] = {0, 3, 6, 9, 12, 15, 18, 21};
int32_t idx_y_data[] = {1, 4, 7, 10, 13, 16, 19, 22};
int32_t idx_z_data[] = {2, 5, 8, 11, 14, 17, 20, 23};

void* centroid_AoS_i(void* arg) {
    UNPACK_ARGS_AoS(arg);

    __m256i sum_x_vec = _mm256_setzero_si256();
    __m256i sum_y_vec = _mm256_setzero_si256();
    __m256i sum_z_vec = _mm256_setzero_si256();
    int32_t sum_x = 0;
    int32_t sum_y = 0;
    int32_t sum_z = 0;
    size_t i;
    // handle remainder first
    for(i = 0; i < (numPoints & 0b111); i++) {
        sum_x += AoS[i].x;
        sum_y += AoS[i].y;
        sum_z += AoS[i].z;
    }

    __m256i idx_x = _mm256_load_si256((__m256i*)idx_x_data);
    __m256i idx_y = _mm256_load_si256((__m256i*)idx_y_data);
    __m256i idx_z = _mm256_load_si256((__m256i*)idx_z_data);

    for(; i < numPoints; i += 8) {

        __m256i xv = _mm256_i32gather_epi32((int32_t*)(AoS + i), idx_x, 4);
        __m256i yv = _mm256_i32gather_epi32((int32_t*)(AoS + i), idx_y, 4);
        __m256i zv = _mm256_i32gather_epi32((int32_t*)(AoS + i), idx_z, 4);

        sum_x_vec = _mm256_add_epi32(sum_x_vec, xv);
        sum_y_vec = _mm256_add_epi32(sum_y_vec, yv);
        sum_z_vec = _mm256_add_epi32(sum_z_vec, zv);
    }
    sum_x += hadd_epi32_256(sum_x_vec);
    sum_y += hadd_epi32_256(sum_y_vec);
    sum_z += hadd_epi32_256(sum_z_vec);

    centroid->x = sum_x / (int32_t)numPoints;
    centroid->y = sum_y / (int32_t)numPoints;
    centroid->z = sum_z / (int32_t)numPoints;
    return NULL;
}
void* centroid_AoS_a(void* arg);

void* centroid_SoA_c(void* arg) {
    UNPACK_ARGS_SoA(arg);

    int32_t sum_x = 0;
    int32_t sum_y = 0;
    int32_t sum_z = 0;
    for(size_t i = 0; i < SoA->numPoints; i++) {
        sum_x += SoA->x[i];
        sum_y += SoA->y[i];
        sum_z += SoA->z[i];
    }
    centroid->x = sum_x / (int32_t)SoA->numPoints;
    centroid->y = sum_y / (int32_t)SoA->numPoints;
    centroid->z = sum_z / (int32_t)SoA->numPoints;

    return NULL;
}

void* centroid_SoA_i(void* arg) {
    UNPACK_ARGS_SoA(arg);

    __m256i sum_x_vec = _mm256_setzero_si256();
    __m256i sum_y_vec = _mm256_setzero_si256();
    __m256i sum_z_vec = _mm256_setzero_si256();
    int32_t sum_x = 0;
    int32_t sum_y = 0;
    int32_t sum_z = 0;
    size_t i;
    // handle remainder first
    for(i = 0; i < (SoA->numPoints & 0b111); i++) {
        sum_x += SoA->x[i];
        sum_y += SoA->y[i];
        sum_z += SoA->z[i];
    }
    // for some reason the compiler puts a ton of jumps here

    for(; i < SoA->numPoints; i += 8) {
        sum_x_vec = _mm256_add_epi32(
            sum_x_vec, _mm256_loadu_si256((__m256i*)(SoA->x + i)));

        sum_y_vec = _mm256_add_epi32(
            sum_y_vec, _mm256_loadu_si256((__m256i*)(SoA->y + i)));

        sum_z_vec = _mm256_add_epi32(
            sum_z_vec, _mm256_loadu_si256((__m256i*)(SoA->z + i)));
    }
    sum_x += hadd_epi32_256(sum_x_vec);
    sum_y += hadd_epi32_256(sum_y_vec);
    sum_z += hadd_epi32_256(sum_z_vec);

    centroid->x = sum_x / (int32_t)SoA->numPoints;
    centroid->y = sum_y / (int32_t)SoA->numPoints;
    centroid->z = sum_z / (int32_t)SoA->numPoints;
    return NULL;
}

void* centroid_SoA_a(void* arg);

void* centroid_AoSoA_c(void* arg) {
    UNPACK_ARGS_AoSoA(arg);

    int32_t sum_x = 0;
    int32_t sum_y = 0;
    int32_t sum_z = 0;
    size_t numStructs = (numPoints + (AoSoA_TileSize - 1)) / AoSoA_TileSize;
    for(size_t i = 0; i < numStructs; i++) {
        for(size_t j = 0; j < AoSoA[i].numPointsUsed; j++) {
            sum_x += AoSoA[i].x[j];
            sum_y += AoSoA[i].y[j];
            sum_z += AoSoA[i].z[j];
        }
    }
    centroid->x = sum_x / (int32_t)numPoints;
    centroid->y = sum_y / (int32_t)numPoints;
    centroid->z = sum_z / (int32_t)numPoints;

    return NULL;
}

void* centroid_AoSoA_i(void* arg) {
    UNPACK_ARGS_AoSoA(arg);

    __m256i sum_x_vec1 = _mm256_setzero_si256();
    __m256i sum_y_vec1 = _mm256_setzero_si256();
    __m256i sum_z_vec1 = _mm256_setzero_si256();
#if AoSoA_TileSize == 16 || AoSoA_TileSize == 32
    __m256i sum_x_vec2 = _mm256_setzero_si256();
    __m256i sum_y_vec2 = _mm256_setzero_si256();
    __m256i sum_z_vec2 = _mm256_setzero_si256();
#endif
    int32_t sum_x = 0;
    int32_t sum_y = 0;
    int32_t sum_z = 0;
    size_t numStructs = (numPoints + (AoSoA_TileSize - 1)) / AoSoA_TileSize;
    size_t i;
    // we will handle all structs but the last one in this loop
    // all structs except last one will be full
    for(i = 0; i < numStructs - 1; i++) {

#if AoSoA_TileSize == 8
        sum_x_vec1 = _mm256_add_epi32(
            sum_x_vec1, _mm256_loadu_si256((__m256i*)(AoSoA[i].x)));
        sum_y_vec1 = _mm256_add_epi32(
            sum_y_vec1, _mm256_loadu_si256((__m256i*)(AoSoA[i].y)));
        sum_z_vec1 = _mm256_add_epi32(
            sum_z_vec1, _mm256_loadu_si256((__m256i*)(AoSoA[i].z)));
#endif
#if AoSoA_TileSize >= 16
        sum_x_vec1 = _mm256_add_epi32(
            sum_x_vec1, _mm256_loadu_si256((__m256i*)(AoSoA[i].x)));
        sum_x_vec2 = _mm256_add_epi32(
            sum_x_vec2, _mm256_loadu_si256((__m256i*)(AoSoA[i].x + 8)));
        sum_y_vec1 = _mm256_add_epi32(
            sum_y_vec1, _mm256_loadu_si256((__m256i*)(AoSoA[i].y)));
        sum_y_vec2 = _mm256_add_epi32(
            sum_y_vec2, _mm256_loadu_si256((__m256i*)(AoSoA[i].y + 8)));
        sum_z_vec1 = _mm256_add_epi32(
            sum_z_vec1, _mm256_loadu_si256((__m256i*)(AoSoA[i].z)));
        sum_z_vec2 = _mm256_add_epi32(
            sum_z_vec2, _mm256_loadu_si256((__m256i*)(AoSoA[i].z + 8)));
#endif
#if AoSoA_TileSize == 32
        sum_x_vec1 = _mm256_add_epi32(
            sum_x_vec1, _mm256_loadu_si256((__m256i*)(AoSoA[i].x + 16)));
        sum_x_vec2 = _mm256_add_epi32(
            sum_x_vec2, _mm256_loadu_si256((__m256i*)(AoSoA[i].x + 24)));
        sum_y_vec1 = _mm256_add_epi32(
            sum_y_vec1, _mm256_loadu_si256((__m256i*)(AoSoA[i].y + 16)));
        sum_y_vec2 = _mm256_add_epi32(
            sum_y_vec2, _mm256_loadu_si256((__m256i*)(AoSoA[i].y + 24)));
        sum_z_vec1 = _mm256_add_epi32(
            sum_z_vec1, _mm256_loadu_si256((__m256i*)(AoSoA[i].z + 16)));
        sum_z_vec2 = _mm256_add_epi32(
            sum_z_vec2, _mm256_loadu_si256((__m256i*)(AoSoA[i].z + 24)));
#endif

    }
#if AoSoA_TileSize == 16 || AoSoA_TileSize == 32
    sum_x_vec1 = _mm256_add_epi32(sum_x_vec1, sum_x_vec2);
    sum_y_vec1 = _mm256_add_epi32(sum_y_vec1, sum_y_vec2);
    sum_z_vec1 = _mm256_add_epi32(sum_z_vec1, sum_z_vec2);
#endif
    sum_x += hadd_epi32_256(sum_x_vec1);
    sum_y += hadd_epi32_256(sum_y_vec1);
    sum_z += hadd_epi32_256(sum_z_vec1);

    // handle final structs
    //ideally we could use mask instructions
    //but thats avx512 which we dont have
    for(; i < numStructs; i++) {
        for(size_t j = 0; j < AoSoA[i].numPointsUsed; j++) {
            sum_x += AoSoA[i].x[j];
            sum_y += AoSoA[i].y[j];
            sum_z += AoSoA[i].z[j];
        }
    }

    centroid->x = sum_x / (int32_t)numPoints;
    centroid->y = sum_y / (int32_t)numPoints;
    centroid->z = sum_z / (int32_t)numPoints;
    return NULL;
}

void* centroid_AoSoA_a(void* arg);

int main() {

    Point* centroid = (Point*)aligned_alloc(64, sizeof(Point));

    Point_AoS* AoS = NULL;
    size_t AoS_numPoints = loadPointsAoS("points.dat", &AoS);
    test_args_AoS_t arg_AoS = {
        .AoS = AoS, .numPoints = AoS_numPoints, .centroid = centroid};

    Point_SoA* SoA = loadPointsSoA("points.dat");
    test_args_SoA_t arg_SoA = {.SoA = SoA, .centroid = centroid};

    Point_AoSoA* AoSoA = NULL;
    size_t AoSoA_numPoints = loadPointsAoSoA("points.dat", &AoSoA);
    test_args_AoSoA_t arg_AoSoA = {
        .AoSoA = AoSoA, .numPoints = AoSoA_numPoints, .centroid = centroid};

#define ALGOS(V)                                                               \
    V(centroid_AoS_c, arg_AoS)                                                 \
    V(centroid_AoS_i, arg_AoS)                                                 \
    V(centroid_AoS_a, arg_AoS)                                                 \
    V(centroid_SoA_c, arg_SoA)                                                 \
    V(centroid_SoA_i, arg_SoA)                                                 \
    V(centroid_SoA_a, arg_SoA)                                                 \
    V(centroid_AoSoA_c, arg_AoSoA)                                             \
    V(centroid_AoSoA_i, arg_AoSoA)                                         \
    V(centroid_AoSoA_a, arg_AoSoA)

#define TEST_ACCURACY(name, arg)                                               \
    memset(centroid, 0, sizeof(Point));                                        \
    name(&arg);                                                                \
    dumpCentroid(#name ".dat", centroid);

#define TEST_SPEED(name, arg)                                                  \
    printf("%-16s: ", #name);                                                  \
    benchmark(name, (void*)(&arg), 1, 2000, 0b00, NULL);                       \
    benchmark(name, (void*)(&arg), 50, 1000, 0b01, NULL);

    ALGOS(TEST_ACCURACY);
    ALGOS(TEST_SPEED);

}
