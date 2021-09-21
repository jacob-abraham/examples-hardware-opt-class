
#include "timing.h"
#include <immintrin.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#define aligned_alloc(_align, _size) malloc(_size)

typedef struct {
    size_t n;      // rows
    size_t m;      // cols
    uint16_t* elm; // size n*m
} Matrix;

#define IndexToRow(_MatrixPtr, _idx) ((size_t)((_idx) / ((_MatrixPtr)->m)))
#define IndexToCol(_MatrixPtr, _idx) ((size_t)((_idx) % ((_MatrixPtr)->m)))
#define RowColToIndex(_MatrixPtr, _row, _col)                                  \
    ((size_t)(((_row) * ((_MatrixPtr)->m)) + (_col)))

#define IndexMatrix(_MatrixPtr, _row, _col)                                    \
    (((_MatrixPtr)->elm)[RowColToIndex(_MatrixPtr, _row, _col)])

void printMatrix(Matrix* m) {
    for(size_t r = 0; r < m->n; r++) {
        for(size_t c = 0; c < m->m; c++) {
            printf("%d ", (int)IndexMatrix(m, r, c));
        }
        printf("\n");
    }
}

// reads file and allocates memory for new Matrix object
#define BUF_SIZE 3000
Matrix* loadMatrix(char* filename) {
    FILE* fp = fopen(filename, "r");
    if(!fp)
        return NULL;
    // allocate matrix
    Matrix* mat = (Matrix*)aligned_alloc(64, sizeof(Matrix));
    // read the first line, contains size information
    size_t ret = fscanf(fp, "%zux%zu\n", &(mat->n), &(mat->m));
    if(ret != 2)
        return NULL;
    mat->elm = (uint16_t*)aligned_alloc(64, sizeof(uint16_t) * mat->n * mat->m);

    char buf[BUF_SIZE];
    for(size_t current_row = 0; current_row < mat->n; current_row++) {
        fgets(buf, BUF_SIZE, fp);
        char* tok = strtok(buf, ",");
        size_t current_col = 0;
        while(tok) {
            IndexMatrix(mat, current_row, current_col) = (uint16_t)atoi(tok);
            current_col++;
            tok = strtok(NULL, ",");
        }
    }
    fclose(fp);
    return mat;
}
#undef BUF_SIZE
void dumpMatrix(char* filename, Matrix* mat) {
    FILE* fp = fopen(filename, "w");
    if(!fp)
        return;

    fprintf(fp, "%zux%zu\n", mat->n, mat->m);
    for(size_t r = 0; r < mat->n; r++) {
        for(size_t c = 0; c < mat->m - 1; c++) {
            fprintf(fp, "%u,", IndexMatrix(mat, r, c));
        }
        fprintf(fp, "%u\n", IndexMatrix(mat, r, mat->m - 1));
    }
    fclose(fp);
}

typedef struct {
    Matrix* A; // nxm
    Matrix* B; // mxp
    Matrix* C; // nxp
} test_args_t;
#define UNPACK_ARGS(ARGS)                                                      \
    Matrix* A = ((test_args_t*)ARGS)->A;                                       \
    Matrix* B = ((test_args_t*)ARGS)->B;                                       \
    Matrix* C = ((test_args_t*)ARGS)->C;

void* matrix_multiply1(void* arg) {
    UNPACK_ARGS(arg)
    size_t n = A->n;
    size_t m = A->m;
    size_t p = B->m;
    for(size_t i = 0; i < n; i++) {
        for(size_t j = 0; j < p; j++) {
            uint16_t sum = 0;
            for(size_t k = 0; k < m; k++) {
                sum += IndexMatrix(A, i, k) * IndexMatrix(B, k, j);
            }
            IndexMatrix(C, i, j) = sum;
        }
    }
    return NULL;
}

#define TILED_MUL(NAME, BLOCKSIZE)                                             \
    void* matrix_##NAME(void* arg) {                                           \
        UNPACK_ARGS(arg)                                                       \
        size_t n = A->n;                                                       \
        size_t m = A->m;                                                       \
        size_t p = B->m;                                                       \
        for(size_t i = 0; i < n; i += BLOCKSIZE) {                             \
            for(size_t j = 0; j < p; j += BLOCKSIZE) {                         \
                for(size_t k = 0; k < m; k += BLOCKSIZE) {                     \
                    for(size_t ii = i; (ii < (i + BLOCKSIZE)) && (ii < n);     \
                        ii++) {                                                \
                        for(size_t jj = j; (jj < (j + BLOCKSIZE)) && (jj < p); \
                            jj++) {                                            \
                            uint16_t sum = 0;                                  \
                            for(size_t kk = k;                                 \
                                (kk < (k + BLOCKSIZE)) && (kk < m); kk++) {    \
                                sum += IndexMatrix(A, ii, kk) *                \
                                       IndexMatrix(B, kk, jj);                 \
                            }                                                  \
                            IndexMatrix(C, ii, jj) += sum;                     \
                        }                                                      \
                    }                                                          \
                }                                                              \
            }                                                                  \
        }                                                                      \
        return NULL;                                                           \
    }

TILED_MUL(multiply2, 4);
TILED_MUL(multiply3, 8);
TILED_MUL(multiply4, 16);
TILED_MUL(multiply5, 32);

/*
A  B  C  D
    +
C  D  A  B
    =
AC BD CA DB
    +
DB CA BD AC
    =
ACDB BDCA CABD DBAC
*/
__attribute((always_inline)) uint16_t dp_4x4(__m128i a, __m128i b) {

    __m128i c = _mm_mullo_epi32(a, b);
    __m128i shuf = _mm_shuffle_epi32(c, 0b01001110);
    c = _mm_add_epi32(c, shuf);
    shuf = _mm_shuffle_epi32(c, 0b00011011);
    c = _mm_add_epi32(c, shuf);
    return (uint16_t)_mm_cvtsi128_si32(c);
}

__attribute((always_inline)) uint16_t dp_4x42(__m128i a, __m128i b) {

    __m128i c = _mm_madd_epi16(a, b);

    __m128i shuf = _mm_shuffle_epi32(c, 0b01001110);
    c = _mm_add_epi32(c, shuf);
    shuf = _mm_shuffle_epi32(c, 0b00011011);
    c = _mm_add_epi32(c, shuf);

    // __m128i shuf = _mm_shuffle_epi32(c, 0b11110000);
    // c = _mm_add_epi32(c, shuf);
    return (uint16_t)_mm_cvtsi128_si32(c);
}

/*
[A00 A01 A02 A03]   [B00 B01 B02 B03]
[A10 A11 A12 A13]   [B10 B11 B12 B13]
[A20 A21 A22 A23]   [B20 B21 B22 B23]
[A30 A31 A32 A33]   [B30 B31 B32 B33]

C00 = A00*B00 + A01*B10 + A02*B20 + A03*B30

*/
__attribute((always_inline)) void
matrix_4x4_mul(Matrix* A, Matrix* B, Matrix* C, size_t i, size_t j, size_t k) {
    // load each of the 4 a rows, this is a trivial operation
    // variables naming, X stands for all. we are loading the entire row
    // this actually loads more than we are using, only using first 4 spots
    __m128i A0X = _mm_cvtepi16_epi32(
        _mm_loadu_si128((__m128i*)(A->elm + RowColToIndex(A, i, k))));
    __m128i A1X = _mm_cvtepi16_epi32(
        _mm_loadu_si128((__m128i*)(A->elm + RowColToIndex(A, i, k + 1))));
    __m128i A2X = _mm_cvtepi16_epi32(
        _mm_loadu_si128((__m128i*)(A->elm + RowColToIndex(A, i, k + 2))));
    __m128i A3X = _mm_cvtepi16_epi32(
        _mm_loadu_si128((__m128i*)(A->elm + RowColToIndex(A, i, k + 3))));

    // load the B columns, this is harder
    union {
        __m128i vec;
        uint32_t i4x[8];
    } split;
#define _X(_i, _j) split.i4x[_i] = IndexMatrix(B, k + _i, j + _j);
#define _R(_j) _X(0, _j) _X(1, _j) _X(2, _j) _X(3, _j)

    _R(0);
    __m128i BX0 = split.vec;
    _R(1);
    __m128i BX1 = split.vec;
    _R(2);
    __m128i BX2 = split.vec;
    _R(3);
    __m128i BX3 = split.vec;
#undef _X
#undef _Z
#undef _R

#define STORE_DP(_istep, _jstep)                                               \
    IndexMatrix(C, i + _istep, j + _jstep) += dp_4x4(A##_istep##X, BX##_jstep);

#define STORE_DP2(_istep)                                                      \
    STORE_DP(_istep, 0);                                                       \
    STORE_DP(_istep, 1);                                                       \
    STORE_DP(_istep, 2);                                                       \
    STORE_DP(_istep, 3);

    STORE_DP2(0);
    STORE_DP2(1);
    STORE_DP2(2);
    STORE_DP2(3);
#undef STORE_DP
#undef STORE_DP2
}

__attribute((always_inline)) void
matrix_4x4_mul2(Matrix* A, Matrix* B, Matrix* C, size_t i, size_t j, size_t k) {
    // load each of the 4 a rows, this is a trivial operation
    // variables naming, X stands for all. we are loading the entire row
    // this actually loads more than we are using, only using first 4 spots
    __m128i A0X = _mm_loadu_si128((__m128i*)(A->elm + RowColToIndex(A, i, k)));
    __m128i A1X =
        _mm_loadu_si128((__m128i*)(A->elm + RowColToIndex(A, i, k + 1)));
    __m128i A2X =
        _mm_loadu_si128((__m128i*)(A->elm + RowColToIndex(A, i, k + 2)));
    __m128i A3X =
        _mm_loadu_si128((__m128i*)(A->elm + RowColToIndex(A, i, k + 3)));

    // load the B columns, this is harder
    union {
        __m128i vec;
        uint16_t i8x[8];
    } split;
#define _X(_i, _j) split.i8x[_i] = IndexMatrix(B, k + _i, j + _j);
#define _Z(_i) split.i8x[_i] = 0;
#define _R(_j) _X(0, _j) _X(1, _j) _X(2, _j) _X(3, _j) _Z(4) _Z(5) _Z(6) _Z(7)

    _R(0);
    __m128i BX0 = split.vec;
    _R(1);
    __m128i BX1 = split.vec;
    _R(2);
    __m128i BX2 = split.vec;
    _R(3);
    __m128i BX3 = split.vec;
#undef _X
#undef _Z
#undef _R

#define STORE_DP(_istep, _jstep)                                               \
    IndexMatrix(C, i + _istep, j + _jstep) += dp_4x42(A##_istep##X, BX##_jstep);

#define STORE_DP2(_istep)                                                      \
    STORE_DP(_istep, 0);                                                       \
    STORE_DP(_istep, 1);                                                       \
    STORE_DP(_istep, 2);                                                       \
    STORE_DP(_istep, 3);

    STORE_DP2(0);
    STORE_DP2(1);
    STORE_DP2(2);
    STORE_DP2(3);
#undef STORE_DP
#undef STORE_DP2
}

void* matrix_multiply6(void* arg) {
    UNPACK_ARGS(arg)
    size_t n = A->n;
    size_t m = A->m;
    size_t p = B->m;
    size_t i, j, k;
    for(i = 0; i < n; i += 4) {
        for(j = 0; j < p; j += 4) {
            for(k = 0; k < (m - 4); k += 4) {
                for(size_t ii = i; (ii < (i + 4)) && (ii < n); ii++) {
                    for(size_t jj = j; (jj < (j + 4)) && (jj < p); jj++) {

                        uint16_t sum =
                            IndexMatrix(A, ii, k) * IndexMatrix(B, k, jj);
                        sum += IndexMatrix(A, ii, k + 1) *
                               IndexMatrix(B, k + 1, jj);
                        sum += IndexMatrix(A, ii, k + 2) *
                               IndexMatrix(B, k + 2, jj);
                        sum += IndexMatrix(A, ii, k + 3) *
                               IndexMatrix(B, k + 3, jj);
                        IndexMatrix(C, ii, jj) += sum;
                    }
                }
            }
            //have to handle remainders
            for(size_t ii = i; (ii < (i + 4)) && (ii < n); ii++) {
                for(size_t jj = j; (jj < (j + 4)) && (jj < p); jj++) {
                    uint16_t sum = 0;
                    for(size_t kk = k; (kk < (k + 4)) && (kk < m); kk++) {
                        sum += IndexMatrix(A, ii, kk) * IndexMatrix(B, kk, jj);
                    }
                    IndexMatrix(C, ii, jj) += sum;
                }
            }
        }
    }

    return NULL;
}

void* matrix_multiply7(void* arg) {
    UNPACK_ARGS(arg)
    size_t n = A->n;
    size_t m = A->m;
    size_t p = B->m;
    size_t i, j, k;
    for(i = 0; i < (n - 4); i += 4) {
        for(j = 0; j < (p - 4); j += 4) {
            for(k = 0; k < (m - 4); k += 4) {

#define UNROLL(_istep, _jstep)                                                 \
    uint16_t sum##_istep##_jstep =                                             \
        IndexMatrix(A, i + _istep, k) * IndexMatrix(B, k, j + _jstep);         \
    sum##_istep##_jstep +=                                                     \
        IndexMatrix(A, i + _istep, k + 1) * IndexMatrix(B, k + 1, j + _jstep); \
    sum##_istep##_jstep +=                                                     \
        IndexMatrix(A, i + _istep, k + 2) * IndexMatrix(B, k + 2, j + _jstep); \
    sum##_istep##_jstep +=                                                     \
        IndexMatrix(A, i + _istep, k + 3) * IndexMatrix(B, k + 3, j + _jstep); \
    IndexMatrix(C, i + _istep, j + _jstep) += sum##_istep##_jstep;

#define UNROLL2(_istep)                                                        \
    UNROLL(_istep, 0);                                                         \
    UNROLL(_istep, 1);                                                         \
    UNROLL(_istep, 2);                                                         \
    UNROLL(_istep, 3);

                UNROLL2(0);
                UNROLL2(1);
                UNROLL2(2);
                UNROLL2(3);

#undef UNROLL
#undef UNROLL2
            }
        }
    }
    for(; i < n; i++) {
        for(; j < p; j++) {
            uint16_t sum = 0;
            for(; k < m; k++) {
                sum += IndexMatrix(A, i, k) * IndexMatrix(B, k, j);
            }
            IndexMatrix(C, i, j) += sum;
        }
    }
    return NULL;
}

void* matrix_multiply8(void* arg) {
    UNPACK_ARGS(arg)
    size_t n = A->n;
    size_t m = A->m;
    size_t p = B->m;
    size_t i, j, k;
    for(i = 0; i < (n - 4); i += 4) {
        for(j = 0; j < (p - 4); j += 4) {
            for(k = 0; k < (m - 4); k += 4) {
                matrix_4x4_mul(A, B, C, i, j, k);
            }
        }
    }

    for(; i < n; i++) {
        for(; j < p; j++) {
            uint16_t sum = 0;
            for(; k < m; k++) {
                sum += IndexMatrix(A, i, k) * IndexMatrix(B, k, j);
            }
            IndexMatrix(C, i, j) += sum;
        }
    }
    return NULL;
}

void* matrix_multiply9(void* arg) {
    UNPACK_ARGS(arg)
    size_t n = A->n;
    size_t m = A->m;
    size_t p = B->m;
    size_t i, j, k;
    for(i = 0; i < (n - 4); i += 4) {
        for(j = 0; j < (p - 4); j += 4) {
            for(k = 0; k < (m - 4); k += 4) {
                matrix_4x4_mul2(A, B, C, i, j, k);
            }
        }
    }

    for(; i < n; i++) {
        for(; j < p; j++) {
            uint16_t sum = 0;
            for(; k < m; k++) {
                sum += IndexMatrix(A, i, k) * IndexMatrix(B, k, j);
            }
            IndexMatrix(C, i, j) += sum;
        }
    }
    return NULL;
}

int main() {

    Matrix* A = loadMatrix("A.dat");
    Matrix* B = loadMatrix("B.dat");
    Matrix* C = (Matrix*)aligned_alloc(64, sizeof(Matrix));
    C->n = A->n;
    C->m = B->m;
    C->elm = (uint16_t*)aligned_alloc(64, sizeof(uint16_t) * C->n * C->m);
    test_args_t arg = {.A = A, .B = B, .C = C};

#define ALGOS(V)                                                               \
    V(multiply1)                                                               \
    V(multiply2)                                                               \
    V(multiply3)                                                               \
    V(multiply4)                                                               \
    V(multiply5)                                                               \
    V(multiply6)                                                               \
    V(multiply7)                                                               \
    V(multiply8)                                                               \
    V(multiply9)

#define TEST_ACCURACY(name)                                                    \
    memset(C->elm, 0, sizeof(uint16_t) * C->n * C->m);                         \
    matrix_##name(&arg);                                                       \
    dumpMatrix(#name ".dat", C);

#define TEST_SPEED(name)                                                       \
    printf("%-10s: ", #name);                                                  \
    benchmark(matrix_##name, (void*)(&arg), 1, 10, 0b00, NULL);                \
    benchmark(matrix_##name, (void*)(&arg), 50, 5, 0b01, NULL);

    // TEST_ACCURACY(multiply6)
    // TEST_ACCURACY(multiply66)
    ALGOS(TEST_ACCURACY)
    ALGOS(TEST_SPEED)
}
