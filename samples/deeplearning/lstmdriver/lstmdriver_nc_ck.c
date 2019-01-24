/******************************************************************************
** Copyright (c) 2018-2019, Intel Corporation                                **
** All rights reserved.                                                      **
**                                                                           **
** Redistribution and use in source and binary forms, with or without        **
** modification, are permitted provided that the following conditions        **
** are met:                                                                  **
** 1. Redistributions of source code must retain the above copyright         **
**    notice, this list of conditions and the following disclaimer.          **
** 2. Redistributions in binary form must reproduce the above copyright      **
**    notice, this list of conditions and the following disclaimer in the    **
**    documentation and/or other materials provided with the distribution.   **
** 3. Neither the name of the copyright holder nor the names of its          **
**    contributors may be used to endorse or promote products derived        **
**    from this software without specific prior written permission.          **
**                                                                           **
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS       **
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT         **
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR     **
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT      **
** HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,    **
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED  **
** TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR    **
** PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF    **
** LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING      **
** NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS        **
** SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.              **
******************************************************************************/
/* Kunal Banerjee (Intel Corp.)
******************************************************************************/
#include <libxsmm.h>
#include <libxsmm_intrinsics_x86.h>

#if defined(LIBXSMM_OFFLOAD_TARGET)
# pragma offload_attribute(push,target(LIBXSMM_OFFLOAD_TARGET))
#endif
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#if defined(_OPENMP)
# include <omp.h>
#endif
#if defined(LIBXSMM_OFFLOAD_TARGET)
# pragma offload_attribute(pop)
#endif

#define CHKERR_LIBXSMM_DNN(A) if ( A != LIBXSMM_DNN_SUCCESS ) fprintf(stderr, "%s\n", libxsmm_dnn_get_error(A) );
/* #define TWO_GEMMS */

LIBXSMM_INLINE void zero_buf(float* buf, size_t size) {
  int i;
#if defined(_OPENMP)
# pragma omp parallel for private(i)
#endif
  for (i = 0; i < (int)size; ++i) {
    buf[i] = 0.0f;
  }
}


LIBXSMM_INLINE void matrix_add(int size, float *a, float *b, float *c)
{
  int i;
#if defined(_OPENMP)
# pragma omp parallel for private(i)
#endif
  for (i = 0; i < size; i++) {
    c[i] = a[i] + b[i];
  }
}


LIBXSMM_INLINE void matrix_eltwise_mult(int size, float *a, float *b, float *c)
{
  int i;
#if defined(_OPENMP)
# pragma omp parallel for private(i)
#endif
  for (i = 0; i < size; i++) {
    c[i] = a[i] * b[i];
  }
}


LIBXSMM_INLINE void matrix_eltwise_mult_ld_a(int m, int n, int ld, float *a, float *b, float *c)
{
  int i;
#if defined(_OPENMP)
# pragma omp parallel for private(i)
#endif
  for (i = 0; i < m*n; i++) {
    int row = i / m;
    int col = i % m;
    c[i] = a[row*ld + col] * b[i];
  }
}


LIBXSMM_INLINE void matrix_eltwise_mult_ld_ab(int m, int n, int ld, float *a, float *b, float *c)
{
  int i;
#if defined(_OPENMP)
# pragma omp parallel for private(i)
#endif
  for (i = 0; i < m*n; i++) {
    int row = i / m;
    int col = i % m;
    c[i] = a[row*ld + col] * b[row*ld + col];
  }
}


LIBXSMM_INLINE void matrix_eltwise_mult_ld_c(int m, int n, int ld, float *a, float *b, float *c)
{
  int i;
#if defined(_OPENMP)
# pragma omp parallel for private(i)
#endif
  for (i = 0; i < m*n; i++) {
    int row = i / m;
    int col = i % m;
    c[row*ld + col] = a[i] * b[i];
  }
}


LIBXSMM_INLINE void matrix_sigmoid(int size, float *src, float *dst)
{
  int i;
#if defined(_OPENMP)
# pragma omp parallel for private(i)
#endif
  for (i = 0; i < size; i++) {
    const float exp_value = (float)exp((double) -src[i]);
    dst[i] = 1.0f / (1.0f + exp_value);
  }
}


LIBXSMM_INLINE void matrix_sigmoid_ld(int m, int n, int ld, float *src, float *dst)
{
  int i;
#if defined(_OPENMP)
# pragma omp parallel for private(i)
#endif
  for (i = 0; i < m*n; i++) {
    int row = i / m;
    int col = i % m;
    const float exp_value = (float)exp((double) -src[row*ld + col]);
    dst[row*ld + col] = 1.0f / (1.0f + exp_value);
  }
}


LIBXSMM_INLINE void matrix_tanh(int size, float *src, float *dst)
{
  int i;
#if defined(_OPENMP)
# pragma omp parallel for private(i)
#endif
  for (i = 0; i < size; i++) {
    dst[i] = (float)tanh((double)src[i]);
  }
}


LIBXSMM_INLINE void matrix_tanh_ld(int m, int n, int ld, float *src, float *dst)
{
  int i;
#if defined(_OPENMP)
# pragma omp parallel for private(i)
#endif
  for (i = 0; i < m*n; i++) {
    int row = i / m;
    int col = i % m;
    dst[row*ld + col] = (float)tanh((double)src[row*ld + col]);
  }
}


LIBXSMM_INLINE void matrix_relu(int size, float *src, float *dst)
{
  int i;
#if defined(_OPENMP)
# pragma omp parallel for private(i)
#endif
  for (i = 0; i < size; i++) {
    dst[i] = (src[i] > 0.0f) ? src[i] : 0.0f;
  }
}


LIBXSMM_INLINE void matrix_sigmoid_inverse(int size, float *src, float *dst)
{
  int i;
#if defined(_OPENMP)
# pragma omp parallel for private(i)
#endif
  for (i = 0; i < size; i++) {
    const float exp_value = (float)exp((double) -src[i]);
    const float sig_exp = 1.0f / (1.0f + exp_value);
    dst[i] = (1.0f - sig_exp)*sig_exp;
  }
}


LIBXSMM_INLINE void matrix_tanh_inverse(int size, float *src, float *dst)
{
  int i;
#if defined(_OPENMP)
# pragma omp parallel for private(i)
#endif
  for (i = 0; i < size; i++) {
    const float tanh_value = (float)tanh((double)src[i]);
    dst[i] = 1.0f - (tanh_value * tanh_value);
  }
}


LIBXSMM_INLINE void matrix_relu_inverse(int size, float *src, float *dst)
{
  int i;
#if defined(_OPENMP)
# pragma omp parallel for private(i)
#endif
  for (i = 0; i < size; i++) {
    dst[i] = (src[i] > 0.0f) ? 1.0f : 0.0f;
  }
}

LIBXSMM_INLINE void matrix_transpose(int rows, int cols, float *src, float *dst)
{
  libxsmm_otrans_omp(dst, src, sizeof(float), cols, rows, cols/*ldi*/, rows/*ldo*/);
}


LIBXSMM_INLINE void matrix_copy(int size, float *src, float *dst)
{
  int i;
#if defined(_OPENMP)
# pragma omp parallel for private(i)
#endif
  for (i = 0; i < size; i++) {
    dst[i] = src[i];
  }
}


LIBXSMM_INLINE void matrix_copy_ld(int m, int n, int ld, float *src, float *dst)
{
  int i;
#if defined(_OPENMP)
# pragma omp parallel for private(i)
#endif
  for (i = 0; i < m*n; i++) {
    int row = i / m;
    int col = i % m;
    dst[i] = src[row*ld + col];
  }
}


LIBXSMM_INLINE void matrix_copy_bias(int m, int n, int ld, float *src, float *dst)
{
  int i;
#if defined(_OPENMP)
# pragma omp parallel for private(i)
#endif
  for (i = 0; i < m*n; i++) {
    int row = i / m;
    int col = i % m;
    dst[row*ld + col] = src[col];
  }
}


LIBXSMM_INLINE void matrix_complement(int size, float *src, float *dst)
{
  int i;
#if defined(_OPENMP)
# pragma omp parallel for private(i)
#endif
  for (i = 0; i < size; i++) {
    dst[i] = 1.0f - src[i];
  }
}


LIBXSMM_INLINE void matrix_complement_ld(int m, int n, int ld, float *src, float *dst)
{
  int i;
#if defined(_OPENMP)
# pragma omp parallel for private(i)
#endif
  for (i = 0; i < m*n; i++) {
    int row = i / m;
    int col = i % m;
    dst[i] = 1.0f - src[row*ld + col];
  }
}


LIBXSMM_INLINE void matrix_complement_square(int size, float *src, float *dst)
{
  int i;
#if defined(_OPENMP)
# pragma omp parallel for private(i)
#endif
  for (i = 0; i < size; i++) {
    dst[i] = 1.0f - (src[i] * src[i]);
  }
}


LIBXSMM_INLINE void matrix_complement_square_ld(int m, int n, int ld, float *src, float *dst)
{
  int i;
#if defined(_OPENMP)
# pragma omp parallel for private(i)
#endif
  for (i = 0; i < m*n; i++) {
    int row = i / m;
    int col = i % m;
    dst[i] = 1.0f - (src[row*ld + col] * src[row*ld + col]);
  }
}


LIBXSMM_INLINE void convert_ck_c4k(int C, int K, float *src, float *dst)
{
  int x, y;
#if defined(_OPENMP)
# pragma omp parallel for private(x, y)
#endif
  for (y = 0; y < C; y++) {
    for (x = 0; x < K; x++) {
      dst[y*4*K + x] = src[y*K + x];
    }
  }
}


LIBXSMM_INLINE void convert_c4k_4ck(int C, int K, float *src, float *dst)
{
  /* offsets: i--0, c--1, f--2, o--3 */
  int x, y, offset;
#if defined(_OPENMP)
# pragma omp parallel for private(x, y, offset)
#endif
  for (offset = 0; offset < 4; offset++) {
    for (y = 0; y < C; y++) {
      for (x = 0; x < K; x++) {
        dst[offset*C*K + y*K + x] = src[y*4*K + offset*K + x];
      }
    }
  }
}


LIBXSMM_INLINE void convert_nk_nck(int N, int K, int CK, float *src, float *dst)
{
  int x, y;
#if defined(_OPENMP)
# pragma omp parallel for private(x, y)
#endif
  for (y = 0; y < N; y++) {
    for (x = 0; x < K; x++) {
      dst[y*CK + x] = src[y*K + x];
    }
  }
}


void lstm_ref( int N, int C, int K, int t, float forget_bias,
               float *wigold, float *wcgold, float *wfgold, float *wogold,
               float *rigold, float *rcgold, float *rfgold, float *rogold,
               float *bigold, float *bcgold, float *bfgold, float *bogold,
               float *xgoldt, float *cspgold, float *hpgold,
               float *csgoldt, float *hgoldt,
               float *dcsgold, float *dhgoldt,
               float *dwgold, float *drgold, float *dbgold,
               float *dxgoldt, float *dcspgold, float *dhpgold )
{
  float *bfgold_fb, *icfogoldt, *dicfogoldt;
#if defined(TWO_GEMMS)
  float *w4gold, *r4gold;
#else
  float *wr8gold, *xhgold, *dxhgold;
#endif
  float *deltagoldt, *doutgoldt;
  float *t1gold, *t2gold, *t3gold;
  const char transa = 'N', transb = 'N';   /* no transposes */
  const char transaT = 'T', transbT = 'T'; /* transposes */
  const float alpha = 1, beta = 1, beta0 = 0;
  int j, l, p;
  int K4 = K * 4;
  int CK = C + K;

  bfgold_fb = (float*)libxsmm_aligned_malloc(K*sizeof(float), 2097152);
  icfogoldt = (float*)libxsmm_aligned_malloc(K*N*t*4*sizeof(float), 2097152);
#if defined(TWO_GEMMS)
  w4gold    = (float*)libxsmm_aligned_malloc(C*K*4*sizeof(float), 2097152);
  r4gold    = (float*)libxsmm_aligned_malloc(K*K*4*sizeof(float), 2097152);
#else
  wr8gold   = (float*)libxsmm_aligned_malloc((C+K)*K*4*sizeof(float), 2097152);
  xhgold    = (float*)libxsmm_aligned_malloc((C+K)*N*sizeof(float), 2097152);
  dxhgold   = (float*)libxsmm_aligned_malloc((C+K)*N*sizeof(float), 2097152);
#endif
  dicfogoldt = (float*)libxsmm_aligned_malloc(K*N*t*4*sizeof(float), 2097152);
  doutgoldt  = (float*)libxsmm_aligned_malloc(K*N*t*sizeof(float), 2097152);
  deltagoldt = (float*)libxsmm_aligned_malloc(K*N*t*sizeof(float), 2097152);
  t1gold = (float*)libxsmm_aligned_malloc(K*N*sizeof(float), 2097152);
  t2gold = (float*)libxsmm_aligned_malloc(K*N*sizeof(float), 2097152);
  t3gold = (float*)libxsmm_aligned_malloc(K*N*sizeof(float), 2097152);
  LIBXSMM_VLA_DECL(2, float, xgold, xgoldt, N * C);
  LIBXSMM_VLA_DECL(2, float, csgold, csgoldt, K * N);
  LIBXSMM_VLA_DECL(2, float, hgold, hgoldt, K * N);
  LIBXSMM_VLA_DECL(3, float, icfogold, icfogoldt, N, 4 * K);
  LIBXSMM_VLA_DECL(2, float, dxgold, dxgoldt, N * C);
  LIBXSMM_VLA_DECL(2, float, dhgold, dhgoldt, K * N);
  LIBXSMM_VLA_DECL(3, float, dicfogold, dicfogoldt, N, 4 * K);
  LIBXSMM_VLA_DECL(2, float, deltagold, deltagoldt, K * N);
  LIBXSMM_VLA_DECL(2, float, doutgold, doutgoldt, K * N);

  /* FWD */
  for (j = 0; j < K; j++) {
    bfgold_fb[j] = bfgold[j] + forget_bias;
  }
#if defined(TWO_GEMMS)
  convert_ck_c4k(C, K, wigold, w4gold);
  convert_ck_c4k(C, K, wcgold, &(w4gold[K]));
  convert_ck_c4k(C, K, wfgold, &(w4gold[2*K]));
  convert_ck_c4k(C, K, wogold, &(w4gold[3*K]));
  convert_ck_c4k(K, K, rigold, r4gold);
  convert_ck_c4k(K, K, rcgold, &(r4gold[K]));
  convert_ck_c4k(K, K, rfgold, &(r4gold[2*K]));
  convert_ck_c4k(K, K, rogold, &(r4gold[3*K]));
#else
  convert_ck_c4k(C, K, wigold, wr8gold);
  convert_ck_c4k(C, K, wcgold, &(wr8gold[K]));
  convert_ck_c4k(C, K, wfgold, &(wr8gold[2*K]));
  convert_ck_c4k(C, K, wogold, &(wr8gold[3*K]));
  convert_ck_c4k(K, K, rigold, &(wr8gold[C*K*4]));
  convert_ck_c4k(K, K, rcgold, &(wr8gold[C*K*4 + K]));
  convert_ck_c4k(K, K, rfgold, &(wr8gold[C*K*4 + 2*K]));
  convert_ck_c4k(K, K, rogold, &(wr8gold[C*K*4 + 3*K]));
#endif
  for (j = 0; j < t; ++j) {
    /* Initialization with bias */
    matrix_copy_bias(K, N, 4*K, bigold,    &LIBXSMM_VLA_ACCESS(3, icfogold, j, 0, 0,   N, 4 * K));
    matrix_copy_bias(K, N, 4*K, bcgold,    &LIBXSMM_VLA_ACCESS(3, icfogold, j, 0, K,   N, 4 * K));
    matrix_copy_bias(K, N, 4*K, bfgold_fb, &LIBXSMM_VLA_ACCESS(3, icfogold, j, 0, 2*K, N, 4 * K));
    matrix_copy_bias(K, N, 4*K, bogold,    &LIBXSMM_VLA_ACCESS(3, icfogold, j, 0, 3*K, N, 4 * K));
#if defined(TWO_GEMMS)
    /* icfo += W * x */
    LIBXSMM_XBLAS_SYMBOL(float)(&transa, &transb, &K4, &N, &C, &alpha, w4gold, &K4, &LIBXSMM_VLA_ACCESS(2, xgold, j, 0, N * C), &C, &beta, &LIBXSMM_VLA_ACCESS(3, icfogold, j, 0, 0, N, 4 * K), &K4);
    /* icfo += R * h */
    if (j == 0) {
      LIBXSMM_XBLAS_SYMBOL(float)(&transa, &transb, &K4, &N, &K, &alpha, r4gold, &K4, hpgold, &K, &beta, &LIBXSMM_VLA_ACCESS(3, icfogold, 0, 0, 0, N, 4 * K), &K4);
    } else {
      LIBXSMM_XBLAS_SYMBOL(float)(&transa, &transb, &K4, &N, &K, &alpha, r4gold, &K4, &LIBXSMM_VLA_ACCESS(2, hgold, j-1, 0, K * N), &K, &beta, &LIBXSMM_VLA_ACCESS(3, icfogold, j, 0, 0, N, 4 * K), &K4);
    }
#else
    /* Concatenate x and h */
    convert_nk_nck(N, C, C+K, &LIBXSMM_VLA_ACCESS(2, xgold, j, 0, N * C), xhgold);
    if (j == 0) {
      convert_nk_nck(N, K, C+K, hpgold, &(xhgold[C]));
    } else {
      convert_nk_nck(N, K, C+K, &LIBXSMM_VLA_ACCESS(2, hgold, j-1, 0, K * N), &(xhgold[C]));
    }
    /* icfo += (W * x) + (R * h) */
    LIBXSMM_XBLAS_SYMBOL(float)(&transa, &transb, &K4, &N, &CK, &alpha, wr8gold, &K4, xhgold, &CK, &beta, &LIBXSMM_VLA_ACCESS(3, icfogold, j, 0, 0, N, 4 * K), &K4);
#endif
    /* icfo = non-lin(icfo) */
    matrix_sigmoid_ld(K, N, 4*K, &LIBXSMM_VLA_ACCESS(3, icfogold, j, 0, 0,   N, 4 * K), &LIBXSMM_VLA_ACCESS(3, icfogold, j, 0, 0,   N, 4 * K));
    matrix_tanh_ld   (K, N, 4*K, &LIBXSMM_VLA_ACCESS(3, icfogold, j, 0, K,   N, 4 * K), &LIBXSMM_VLA_ACCESS(3, icfogold, j, 0, K,   N, 4 * K));
    matrix_sigmoid_ld(K, N, 4*K, &LIBXSMM_VLA_ACCESS(3, icfogold, j, 0, 2*K, N, 4 * K), &LIBXSMM_VLA_ACCESS(3, icfogold, j, 0, 2*K, N, 4 * K));
    matrix_sigmoid_ld(K, N, 4*K, &LIBXSMM_VLA_ACCESS(3, icfogold, j, 0, 3*K, N, 4 * K), &LIBXSMM_VLA_ACCESS(3, icfogold, j, 0, 3*K, N, 4 * K));
    /* t1 = f.cs */
    if (j == 0) {
      matrix_eltwise_mult_ld_a(K, N, 4*K, &LIBXSMM_VLA_ACCESS(3, icfogold, 0, 0, 2*K, N, 4 * K), cspgold, t1gold);
    } else {
      matrix_eltwise_mult_ld_a(K, N, 4*K, &LIBXSMM_VLA_ACCESS(3, icfogold, j, 0, 2*K, N, 4 * K), &LIBXSMM_VLA_ACCESS(2, csgold, j-1, 0, K * N), t1gold);
    }
    /* t2 = i.c */
    matrix_eltwise_mult_ld_ab(K, N, 4*K, &LIBXSMM_VLA_ACCESS(3, icfogold, j, 0, 0,   N, 4 * K), &LIBXSMM_VLA_ACCESS(3, icfogold, j, 0, K, N, 4 * K), t2gold);
    /* cs = t1 + t2 */
    matrix_add(K*N, t1gold, t2gold, &LIBXSMM_VLA_ACCESS(2, csgold, j, 0, K * N));
    /* t3 = tanh(cs) */
    matrix_tanh(K*N, &LIBXSMM_VLA_ACCESS(2, csgold, j, 0, K * N), t3gold);
    /* h = o.t3 */
    matrix_eltwise_mult_ld_a (K, N, 4*K, &LIBXSMM_VLA_ACCESS(3, icfogold, j, 0, 3*K, N, 4 * K), t3gold, &LIBXSMM_VLA_ACCESS(2, hgold, j, 0, K * N));
  }
  /* BWD/UPD */
  for (j = t-1; j >= 0; --j) {
    /* compute delta */
    if (j == t-1) {
      matrix_copy(K * N, &LIBXSMM_VLA_ACCESS(2, dhgold, t-1, 0, K * N), &LIBXSMM_VLA_ACCESS(2, deltagold, t-1, 0, K * N));
    } else {
      matrix_add(K * N, &LIBXSMM_VLA_ACCESS(2, doutgold, j, 0, K * N), &LIBXSMM_VLA_ACCESS(2, dhgold, j, 0, K * N), &LIBXSMM_VLA_ACCESS(2, deltagold, j, 0, K * N));
    }
    /* compute dcsp */
    matrix_eltwise_mult_ld_a(K, N, 4*K, &LIBXSMM_VLA_ACCESS(3, icfogold, j, 0, 3*K, N, 4 * K), &LIBXSMM_VLA_ACCESS(2, deltagold, j, 0, K * N), t1gold);
    matrix_tanh_inverse(K * N, &LIBXSMM_VLA_ACCESS(2, csgold, j, 0, K * N), t2gold);
    matrix_eltwise_mult(K * N, t1gold, t2gold, t3gold);
    if (j == t-1) {
      matrix_add(K * N, t3gold, dcsgold, dcspgold);
    } else {
      matrix_add(K * N, t3gold, dcspgold, dcspgold);
    }
    /* compute dc */
    matrix_eltwise_mult_ld_a   (K, N, 4*K, &LIBXSMM_VLA_ACCESS(3, icfogold, j, 0, 0, N, 4 * K), dcspgold, t1gold);
    matrix_complement_square_ld(K, N, 4*K, &LIBXSMM_VLA_ACCESS(3, icfogold, j, 0, K, N, 4 * K), t2gold);
    matrix_eltwise_mult_ld_c   (K, N, 4*K, t1gold, t2gold, &LIBXSMM_VLA_ACCESS(3, dicfogold, j, 0, K, N, 4 * K));
    /* compute di */
    matrix_eltwise_mult_ld_a   (K, N, 4*K, &LIBXSMM_VLA_ACCESS(3, icfogold, j, 0, K, N, 4 * K), dcspgold, t1gold);
    matrix_complement_ld       (K, N, 4*K, &LIBXSMM_VLA_ACCESS(3, icfogold, j, 0, 0, N, 4 * K), t2gold);
    matrix_eltwise_mult_ld_a   (K, N, 4*K, &LIBXSMM_VLA_ACCESS(3, icfogold, j, 0, 0, N, 4 * K), t2gold, t3gold);
    matrix_eltwise_mult_ld_c   (K, N, 4*K, t1gold, t3gold, &LIBXSMM_VLA_ACCESS(3, dicfogold, j, 0, 0, N, 4 * K));
    /* compute df */
    if (j == 0) {
      matrix_eltwise_mult(K * N, dcspgold, cspgold, t1gold);
    } else {
      matrix_eltwise_mult(K * N, dcspgold, &LIBXSMM_VLA_ACCESS(2, csgold, j-1, 0, K * N), t1gold);
    }
    matrix_complement_ld       (K, N, 4*K, &LIBXSMM_VLA_ACCESS(3, icfogold, j, 0, 2*K, N, 4 * K), t2gold);
    matrix_eltwise_mult_ld_a   (K, N, 4*K, &LIBXSMM_VLA_ACCESS(3, icfogold, j, 0, 2*K, N, 4 * K), t2gold, t3gold);
    matrix_eltwise_mult_ld_c   (K, N, 4*K, t1gold, t3gold, &LIBXSMM_VLA_ACCESS(3, dicfogold, j, 0, 2*K, N, 4 * K));
    /* compute do */
    matrix_tanh(K * N, &LIBXSMM_VLA_ACCESS(2, csgold, j, 0, K * N), t1gold);
    matrix_eltwise_mult(K * N, &LIBXSMM_VLA_ACCESS(2, deltagold, j, 0, K * N), t1gold, t1gold);
    matrix_complement_ld       (K, N, 4*K, &LIBXSMM_VLA_ACCESS(3, icfogold, j, 0, 3*K, N, 4 * K), t2gold);
    matrix_eltwise_mult_ld_a   (K, N, 4*K, &LIBXSMM_VLA_ACCESS(3, icfogold, j, 0, 3*K, N, 4 * K), t2gold, t2gold);
    matrix_eltwise_mult_ld_c   (K, N, 4*K, t1gold, t2gold, &LIBXSMM_VLA_ACCESS(3, dicfogold, j, 0, 3*K, N, 4 * K));
    /* update dcsp */
    matrix_eltwise_mult_ld_a   (K, N, 4*K, &LIBXSMM_VLA_ACCESS(3, icfogold, j, 0, 2*K, N, 4 * K), dcspgold, dcspgold);

#if defined(TWO_GEMMS)
    if (j > 0) {
      /* compute dout */
      LIBXSMM_XBLAS_SYMBOL(float)(&transaT, &transb, &K, &N, &K4, &alpha, r4gold, &K4, &LIBXSMM_VLA_ACCESS(3, dicfogold, j, 0, 0, N, 4 * K), &K4, &beta0, &LIBXSMM_VLA_ACCESS(2, doutgold, j-1, 0, K * N), &K);
    } else {
      /* compute dhp */
      LIBXSMM_XBLAS_SYMBOL(float)(&transaT, &transb, &K, &N, &K4, &alpha, r4gold, &K4, &LIBXSMM_VLA_ACCESS(3, dicfogold, 0, 0, 0, N, 4 * K), &K4, &beta0, dhpgold, &K);
    }

    /* compute dx */
    LIBXSMM_XBLAS_SYMBOL(float)(&transaT, &transb, &C, &N, &K4, &alpha, w4gold, &K4, &LIBXSMM_VLA_ACCESS(3, dicfogold, j, 0, 0, N, 4 * K), &K4, &beta, &LIBXSMM_VLA_ACCESS(2, dxgold, j, 0, N * C), &C);

    /* compute dw */
    LIBXSMM_XBLAS_SYMBOL(float)(&transa, &transbT, &K4, &C, &N, &alpha, &LIBXSMM_VLA_ACCESS(3, dicfogold, j, 0, 0, N, 4 * K), &K4, &LIBXSMM_VLA_ACCESS(2, xgold, j, 0, N * C), &C, &beta, dwgold, &K4);

    /* compute dr */
    if (j == 0) {
      LIBXSMM_XBLAS_SYMBOL(float)(&transa, &transbT, &K4, &K, &N, &alpha, &LIBXSMM_VLA_ACCESS(3, dicfogold, j, 0, 0, N, 4 * K), &K4, hpgold, &K, &beta, drgold, &K4);
    } else {
      LIBXSMM_XBLAS_SYMBOL(float)(&transa, &transbT, &K4, &K, &N, &alpha, &LIBXSMM_VLA_ACCESS(3, dicfogold, j, 0, 0, N, 4 * K), &K4, &LIBXSMM_VLA_ACCESS(2, hgold, j-1, 0, K * N), &K, &beta, drgold, &K4);
    }
#else
    LIBXSMM_XBLAS_SYMBOL(float)(&transaT, &transb, &CK, &N, &K4, &alpha, wr8gold, &K4, &LIBXSMM_VLA_ACCESS(3, dicfogold, j, 0, 0, N, 4 * K), &K4, &beta0, dxhgold, &CK);
    matrix_copy_ld(C, N, C+K, dxhgold, &LIBXSMM_VLA_ACCESS(2, dxgold, j, 0, N * C));
    if (j > 0) {
      matrix_copy_ld(K, N, C+K, &(dxhgold[C]), &LIBXSMM_VLA_ACCESS(2, doutgold, j-1, 0, K * N));
    } else {
      matrix_copy_ld(K, N, C+K, &(dxhgold[C]), dhpgold);
    }

    /* Concatenate x and h */
    convert_nk_nck(N, C, C+K, &LIBXSMM_VLA_ACCESS(2, xgold, j, 0, N * C), xhgold);
    if (j == 0) {
      convert_nk_nck(N, K, C+K, hpgold, &(xhgold[C]));
    } else {
      convert_nk_nck(N, K, C+K, &LIBXSMM_VLA_ACCESS(2, hgold, j-1, 0, K * N), &(xhgold[C]));
    }
    LIBXSMM_XBLAS_SYMBOL(float)(&transa, &transbT, &K4, &CK, &N, &alpha, &LIBXSMM_VLA_ACCESS(3, dicfogold, j, 0, 0, N, 4 * K), &K4, xhgold, &CK, &beta, dwgold, &K4);
#endif

    /* compute db */
#if defined(_OPENMP)
# pragma omp parallel for private(l, p)
#endif
    for (l = 0; l < K; l++) {
      for (p = 0; p < N; p++) {
        dbgold[l]       += LIBXSMM_VLA_ACCESS(3, dicfogold, j, p, l,       N, 4 * K);
        dbgold[l + K]   += LIBXSMM_VLA_ACCESS(3, dicfogold, j, p, l + K,   N, 4 * K);
        dbgold[l + 2*K] += LIBXSMM_VLA_ACCESS(3, dicfogold, j, p, l + 2*K, N, 4 * K);
        dbgold[l + 3*K] += LIBXSMM_VLA_ACCESS(3, dicfogold, j, p, l + 3*K, N, 4 * K);
      }
    }
  }

  libxsmm_free(bfgold_fb);
  libxsmm_free(icfogoldt);
  libxsmm_free(dicfogoldt);
#if defined(TWO_GEMMS)
  libxsmm_free(w4gold);
  libxsmm_free(r4gold);
#else
  libxsmm_free(wr8gold);
  libxsmm_free(xhgold);
  libxsmm_free(dxhgold);
#endif
  libxsmm_free(t1gold);
  libxsmm_free(t2gold);
  libxsmm_free(t3gold);
}


int main(int argc, char* argv[])
{
  float *wigold, *wfgold, *wogold, *wcgold, *rigold, *rfgold, *rogold, *rcgold, *bigold, *bfgold, *bogold, *bcgold;
  float *xgoldt, *cspgold,*hpgold, *csgoldt, *hgoldt;
  float *dwgold, *drgold, *dbgold;
  float *dxgoldt, *dcspgold, *dhpgold, *dcsgold, *dhgoldt;
  float *xt, *csp, *hp, *w, *r, *b, *cst, *ht;
  float *it, *ft, *ot, *cit, *cot;
  float *dxt, *dcsp, *dhp, *dw, *dr, *db, *dcs, *dht;
  float forget_bias = 1.0f;

  void *scratch, *internalstate;
  size_t scratch_size = 0, internalstate_size = 0;

  int iters = 10;   /* repetitions of benchmark */
  int pass = 3;     /* pass: 0--FWD, 1--BWD, 2--UPD, 3--BWD+UPD */
  int N = 128;      /* size of mini-batch */
  int C = 512;      /* number of inputs */
  int K = 64;       /* number of outputs */
  int t = 5;        /* number of time steps (>= 1) */
  int K4 = K * 4;
  int CK = C + K;
  int bk = 64;
  int bn = 64;
  int bc = 64;

  const char *const env_check = getenv("CHECK");
  const double check = LIBXSMM_ABS(0 == env_check ? 0/*disabled by default*/ : atof(env_check));

#if defined(_OPENMP)
  int nThreads = omp_get_max_threads(); /* number of threads */
#else
  int nThreads = 1; /* number of threads */
#endif

  unsigned long long l_start, l_end;
  double l_total = 0.0;
  double flops = 0.0, tempflops = 0.0;
  const double tflops = 12; /* transcendental flops */
  int j;

  libxsmm_dnn_rnncell_desc lstmcell_desc;
  libxsmm_dnn_rnncell* libxsmm_handle;
  libxsmm_dnn_tensor* libxsmm_input;
  libxsmm_dnn_tensor* libxsmm_cs_prev;
  libxsmm_dnn_tensor* libxsmm_hidden_state_prev;
  libxsmm_dnn_tensor* libxsmm_weight;
  libxsmm_dnn_tensor* libxsmm_recur_weight;
  libxsmm_dnn_tensor* libxsmm_bias;
  libxsmm_dnn_tensor* libxsmm_cs;
  libxsmm_dnn_tensor* libxsmm_hidden_state;
  libxsmm_dnn_tensor* libxsmm_i;
  libxsmm_dnn_tensor* libxsmm_f;
  libxsmm_dnn_tensor* libxsmm_o;
  libxsmm_dnn_tensor* libxsmm_ci;
  libxsmm_dnn_tensor* libxsmm_co;
  libxsmm_dnn_tensor* libxsmm_dinput;
  libxsmm_dnn_tensor* libxsmm_dcs_prev;
  libxsmm_dnn_tensor* libxsmm_dhidden_state_prev;
  libxsmm_dnn_tensor* libxsmm_dweight;
  libxsmm_dnn_tensor* libxsmm_drecur_weight;
  libxsmm_dnn_tensor* libxsmm_dbias;
  libxsmm_dnn_tensor* libxsmm_dcs;
  libxsmm_dnn_tensor* libxsmm_dhidden_state;

  libxsmm_dnn_tensor_datalayout* libxsmm_layout;
  libxsmm_dnn_err_t status;
  libxsmm_dnn_err_t global_status = LIBXSMM_DNN_SUCCESS;

  libxsmm_matdiff_info norms_fwd, norms_bwd, norms_upd_w, norms_upd_r, norms_upd_b, diff;
  libxsmm_matdiff_clear(&norms_fwd);
  libxsmm_matdiff_clear(&norms_bwd);
  libxsmm_matdiff_clear(&norms_upd_w);
  libxsmm_matdiff_clear(&norms_upd_r);
  libxsmm_matdiff_clear(&norms_upd_b);
  libxsmm_matdiff_clear(&diff);

  if (argc > 1 && !strncmp(argv[1], "-h", 3)) {
    printf("\nUsage: ./lstmdriver [reps] [pass: 0--FWD, 1--BWD, 2--UPD, 3--BWD+UPD] [N] [C] [K] [time_steps > 0]\n\n");
    return 0;
  }
  libxsmm_srand(1);

  /* reading new values from cli */
  j = 1;
  if (argc > j) iters = atoi(argv[j++]);
  if (argc > j) pass  = atoi(argv[j++]);
  if (argc > j) N     = atoi(argv[j++]);
  if (argc > j) C     = atoi(argv[j++]);
  if (argc > j) K     = atoi(argv[j++]);
  if (argc > j) t     = atoi(argv[j++]);
  if (argc > j) bn     = atoi(argv[j++]);
  if (argc > j) bk     = atoi(argv[j++]);
  if (argc > j) bc     = atoi(argv[j++]);  
  K4 = K * 4;
  CK = C + K;

  if (t <= 0) {
    printf("time_steps %d should be greater than or equal to 1\n\n", t);
    return 0;
  }
  if (!(pass == 0 || pass == 1 || pass == 2 || pass == 3)) {
    printf("Unknown pass: %d, valid arguments for pass = {0(FWD), 1(BWD), 2(UPD), 3(BWD+UPD)\n\n", pass);
    return 0;
  }

#if defined(__SSE3__)
  _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
  _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
  _MM_SET_ROUNDING_MODE(_MM_ROUND_NEAREST);
#endif

  /* print some summary */
  printf("##########################################\n");
  printf("#          Setting Up (Common)           #\n");
  printf("##########################################\n");
  printf("PARAMS: N:%d  C:%d  K:%d  T:%d\n", N, C, K, t);
  printf("PARAMS: ITERS:%d", iters); if (LIBXSMM_FEQ(0, check)) printf("  Threads:%d\n", nThreads); else printf("\n");
  printf("SIZE Weight (MB): %10.2f MiB\n", (double)(C*K*sizeof(float))/(1024.0*1024.0) );
  printf("SIZE Input (MB): %10.2f MiB\n", (double)(N*C*sizeof(float))/(1024.0*1024.0) );
  printf("SIZE Hidden State: %10.2f MiB\n", (double)(K*N*sizeof(float))/(1024.0*1024.0) );

  /* allocate data */
  xgoldt  = (float*)libxsmm_aligned_malloc(N*C*t*sizeof(float), 2097152);
  cspgold = (float*)libxsmm_aligned_malloc(K*N*sizeof(float), 2097152);
  hpgold  = (float*)libxsmm_aligned_malloc(K*N*sizeof(float), 2097152);
  wigold  = (float*)libxsmm_aligned_malloc(C*K*sizeof(float), 2097152);
  wfgold  = (float*)libxsmm_aligned_malloc(C*K*sizeof(float), 2097152);
  wogold  = (float*)libxsmm_aligned_malloc(C*K*sizeof(float), 2097152);
  wcgold  = (float*)libxsmm_aligned_malloc(C*K*sizeof(float), 2097152);
  rigold  = (float*)libxsmm_aligned_malloc(K*K*sizeof(float), 2097152);
  rfgold  = (float*)libxsmm_aligned_malloc(K*K*sizeof(float), 2097152);
  rogold  = (float*)libxsmm_aligned_malloc(K*K*sizeof(float), 2097152);
  rcgold  = (float*)libxsmm_aligned_malloc(K*K*sizeof(float), 2097152);
  bigold  = (float*)libxsmm_aligned_malloc(K*sizeof(float), 2097152);
  bfgold  = (float*)libxsmm_aligned_malloc(K*sizeof(float), 2097152);
  bogold  = (float*)libxsmm_aligned_malloc(K*sizeof(float), 2097152);
  bcgold  = (float*)libxsmm_aligned_malloc(K*sizeof(float), 2097152);
  csgoldt = (float*)libxsmm_aligned_malloc(K*N*t*sizeof(float), 2097152);
  hgoldt  = (float*)libxsmm_aligned_malloc(K*N*t*sizeof(float), 2097152);
  dxgoldt = (float*)libxsmm_aligned_malloc(N*C*t*sizeof(float), 2097152);
  dcspgold= (float*)libxsmm_aligned_malloc(K*N*sizeof(float), 2097152);
  dhpgold = (float*)libxsmm_aligned_malloc(K*N*sizeof(float), 2097152);
#if defined(TWO_GEMMS)
  dwgold  = (float*)libxsmm_aligned_malloc(C*K*4*sizeof(float), 2097152);
  drgold  = (float*)libxsmm_aligned_malloc(K*K*4*sizeof(float), 2097152);
#else
  dwgold  = (float*)libxsmm_aligned_malloc((C+K)*K*4*sizeof(float), 2097152);
  drgold  = NULL;
#endif
  dbgold  = (float*)libxsmm_aligned_malloc(K*4*sizeof(float), 2097152);
  dcsgold = (float*)libxsmm_aligned_malloc(K*N*sizeof(float), 2097152);
  dhgoldt = (float*)libxsmm_aligned_malloc(K*N*t*sizeof(float), 2097152);
  xt      = (float*)libxsmm_aligned_malloc(N*C*t*sizeof(float), 2097152);
  csp     = (float*)libxsmm_aligned_malloc(K*N*sizeof(float), 2097152);
  hp      = (float*)libxsmm_aligned_malloc(K*N*sizeof(float), 2097152);
  w       = (float*)libxsmm_aligned_malloc(C*K*4*sizeof(float), 2097152);
  r       = (float*)libxsmm_aligned_malloc(K*K*4*sizeof(float), 2097152);
  b       = (float*)libxsmm_aligned_malloc(K*4*sizeof(float), 2097152);
  cst     = (float*)libxsmm_aligned_malloc(K*N*t*sizeof(float), 2097152);
  ht      = (float*)libxsmm_aligned_malloc(K*N*t*sizeof(float), 2097152);
  it      = (float*)libxsmm_aligned_malloc(K*N*t*sizeof(float), 2097152);
  ft      = (float*)libxsmm_aligned_malloc(K*N*t*sizeof(float), 2097152);
  ot      = (float*)libxsmm_aligned_malloc(K*N*t*sizeof(float), 2097152);
  cit     = (float*)libxsmm_aligned_malloc(K*N*t*sizeof(float), 2097152);
  cot     = (float*)libxsmm_aligned_malloc(K*N*t*sizeof(float), 2097152);
  dxt     = (float*)libxsmm_aligned_malloc(N*C*t*sizeof(float), 2097152);
  dcsp    = (float*)libxsmm_aligned_malloc(K*N*sizeof(float), 2097152);
  dhp     = (float*)libxsmm_aligned_malloc(K*N*sizeof(float), 2097152);
  dw      = (float*)libxsmm_aligned_malloc(C*K*4*sizeof(float), 2097152);
  dr      = (float*)libxsmm_aligned_malloc(K*K*4*sizeof(float), 2097152);
  db      = (float*)libxsmm_aligned_malloc(K*4*sizeof(float), 2097152);
  dcs     = (float*)libxsmm_aligned_malloc(K*N*sizeof(float), 2097152);
  dht     = (float*)libxsmm_aligned_malloc(K*N*t*sizeof(float), 2097152);
  LIBXSMM_VLA_DECL(2, float, xgold, xgoldt, N * C);
  LIBXSMM_VLA_DECL(2, float, hgold, hgoldt, K * N);
  LIBXSMM_VLA_DECL(2, float, dhgold, dhgoldt, K * N);
  LIBXSMM_VLA_DECL(2, float, h, ht, K * N);

  /* initialize data */
  /* FWD */
  for (j = 0; j < t; ++j) {
    LIBXSMM_MATINIT_OMP(float, 24, &LIBXSMM_VLA_ACCESS(2, xgold, j, 0, N * C), N, C, N, 1.0);
  }
  LIBXSMM_MATINIT_OMP(float, 24, cspgold,N, K, N, 1.0);
  LIBXSMM_MATINIT_OMP(float, 24, hpgold, N, K, N, 1.0);
  LIBXSMM_MATINIT_OMP(float, 42, wigold, C, K, C, 1.0);
  LIBXSMM_MATINIT_OMP(float, 42, wfgold, C, K, C, 1.0);
  LIBXSMM_MATINIT_OMP(float, 42, wogold, C, K, C, 1.0);
  LIBXSMM_MATINIT_OMP(float, 42, wcgold, C, K, C, 1.0);
  LIBXSMM_MATINIT_OMP(float, 42, rigold, K, K, K, 1.0);
  LIBXSMM_MATINIT_OMP(float, 42, rfgold, K, K, K, 1.0);
  LIBXSMM_MATINIT_OMP(float, 42, rogold, K, K, K, 1.0);
  LIBXSMM_MATINIT_OMP(float, 42, rcgold, K, K, K, 1.0);
  LIBXSMM_MATINIT_OMP(float, 24, bigold, 1, K, 1, 1.0);
  LIBXSMM_MATINIT_OMP(float, 24, bfgold, 1, K, 1, 1.0);
  LIBXSMM_MATINIT_OMP(float, 24, bogold, 1, K, 1, 1.0);
  LIBXSMM_MATINIT_OMP(float, 24, bcgold, 1, K, 1, 1.0);
  /* BWD/UPD */
  for (j = 0; j < t; ++j) {
    LIBXSMM_MATINIT_OMP(float, 24, &LIBXSMM_VLA_ACCESS(2, dhgold, j, 0, K * N), N, K, N, 1.0);
  }
  LIBXSMM_MATINIT_OMP(float, 24, dcsgold, N, K, N, 1.0);
  zero_buf(dxgoldt, N*C*t);
  zero_buf(dcspgold, K*N);
  zero_buf(dhpgold, K*N);
#if defined(TWO_GEMMS)
  zero_buf(dwgold, C*K*4);
  zero_buf(drgold, K*K*4);
#else
  zero_buf(dwgold, (C+K)*K*4);
#endif
  zero_buf(dbgold, K*4);

  /* first touch LIBXSMM */
  zero_buf(xt,  N*C*t);
  zero_buf(csp, K*N);
  zero_buf(hp,  K*N);
  zero_buf(w,   C*K*4);
  zero_buf(r,   K*K*4);
  zero_buf(b,   K*4);
  zero_buf(cst, K*N*t);
  zero_buf(ht,  K*N*t);
  zero_buf(it,  K*N*t);
  zero_buf(ft,  K*N*t);
  zero_buf(ot,  K*N*t);
  zero_buf(cit, K*N*t);
  zero_buf(cot, K*N*t);
  zero_buf(dxt,  N*C*t);
  zero_buf(dcsp, K*N);
  zero_buf(dhp,  K*N);
  zero_buf(dw,   C*K*4);
  zero_buf(dr,   K*K*4);
  zero_buf(db,   K*4);
  zero_buf(dcs,  K*N);
  zero_buf(dht,  K*N*t);

  if (LIBXSMM_NEQ(0, check)) {
    printf("##########################################\n");
    printf("#         Computing Reference ...        #\n");
    printf("##########################################\n");

    lstm_ref( N, C, K, t, forget_bias,
              wigold, wcgold, wfgold, wogold,
              rigold, rcgold, rfgold, rogold,
              bigold, bcgold, bfgold, bogold,
              xgoldt, cspgold, hpgold,
              csgoldt, hgoldt,
              dcsgold, dhgoldt,
              dwgold, drgold, dbgold,
              dxgoldt, dcspgold, dhpgold );

    printf("##########################################\n");
    printf("#      Computing Reference ... done      #\n");
    printf("##########################################\n");
  }

  if (1 /* format == 'A' || format == 'L' */) {
    printf("\n");
    printf("##########################################\n");
    printf("#      Setting Up  (custom-Storage)      #\n");
    printf("##########################################\n");

    /* setup LIBXSMM handle */
    lstmcell_desc.threads = nThreads;
    lstmcell_desc.N = N;
    lstmcell_desc.C = C;
    lstmcell_desc.K = K;
    lstmcell_desc.t = t;
    lstmcell_desc.bn = bn;
    lstmcell_desc.bk = bk;
    lstmcell_desc.bc = bc;    
    lstmcell_desc.cell_type = LIBXSMM_DNN_RNNCELL_LSTM;
    lstmcell_desc.datatype_in = LIBXSMM_DNN_DATATYPE_F32;
    lstmcell_desc.datatype_out = LIBXSMM_DNN_DATATYPE_F32;
    lstmcell_desc.buffer_format = LIBXSMM_DNN_TENSOR_FORMAT_NC;
    lstmcell_desc.filter_format = LIBXSMM_DNN_TENSOR_FORMAT_CK;

    libxsmm_handle = libxsmm_dnn_create_rnncell( lstmcell_desc, &status );
    CHKERR_LIBXSMM_DNN( status );
    CHKERR_LIBXSMM_DNN( libxsmm_dnn_rnncell_allocate_forget_bias(libxsmm_handle, forget_bias) );

    /* setup LIBXSMM buffers and filter */
    libxsmm_layout = libxsmm_dnn_rnncell_create_tensor_datalayout( libxsmm_handle, LIBXSMM_DNN_RNN_REGULAR_INPUT, &status ); CHKERR_LIBXSMM_DNN( status );
    libxsmm_input = libxsmm_dnn_link_tensor( libxsmm_layout, xt, &status ); CHKERR_LIBXSMM_DNN( status );
    libxsmm_dnn_destroy_tensor_datalayout( libxsmm_layout );

    libxsmm_layout = libxsmm_dnn_rnncell_create_tensor_datalayout( libxsmm_handle, LIBXSMM_DNN_RNN_REGULAR_CS_PREV, &status ); CHKERR_LIBXSMM_DNN( status );
    libxsmm_cs_prev = libxsmm_dnn_link_tensor( libxsmm_layout, csp, &status ); CHKERR_LIBXSMM_DNN( status );
    libxsmm_dnn_destroy_tensor_datalayout( libxsmm_layout );

    libxsmm_layout = libxsmm_dnn_rnncell_create_tensor_datalayout( libxsmm_handle, LIBXSMM_DNN_RNN_REGULAR_HIDDEN_STATE_PREV, &status ); CHKERR_LIBXSMM_DNN( status );
    libxsmm_hidden_state_prev = libxsmm_dnn_link_tensor( libxsmm_layout, hp, &status ); CHKERR_LIBXSMM_DNN( status );
    libxsmm_dnn_destroy_tensor_datalayout( libxsmm_layout );

    libxsmm_layout = libxsmm_dnn_rnncell_create_tensor_datalayout( libxsmm_handle, LIBXSMM_DNN_RNN_REGULAR_WEIGHT, &status ); CHKERR_LIBXSMM_DNN( status );
    libxsmm_weight = libxsmm_dnn_link_tensor( libxsmm_layout, w, &status ); CHKERR_LIBXSMM_DNN( status );
    libxsmm_dnn_destroy_tensor_datalayout( libxsmm_layout );

    libxsmm_layout = libxsmm_dnn_rnncell_create_tensor_datalayout( libxsmm_handle, LIBXSMM_DNN_RNN_REGULAR_RECUR_WEIGHT, &status ); CHKERR_LIBXSMM_DNN( status );
    libxsmm_recur_weight = libxsmm_dnn_link_tensor( libxsmm_layout, r, &status ); CHKERR_LIBXSMM_DNN( status );
    libxsmm_dnn_destroy_tensor_datalayout( libxsmm_layout );

    libxsmm_layout = libxsmm_dnn_rnncell_create_tensor_datalayout( libxsmm_handle, LIBXSMM_DNN_RNN_REGULAR_BIAS, &status ); CHKERR_LIBXSMM_DNN( status );
    libxsmm_bias = libxsmm_dnn_link_tensor( libxsmm_layout, b, &status ); CHKERR_LIBXSMM_DNN( status );
    libxsmm_dnn_destroy_tensor_datalayout( libxsmm_layout );

    libxsmm_layout = libxsmm_dnn_rnncell_create_tensor_datalayout( libxsmm_handle, LIBXSMM_DNN_RNN_REGULAR_CS, &status ); CHKERR_LIBXSMM_DNN( status );
    libxsmm_cs = libxsmm_dnn_link_tensor( libxsmm_layout, cst, &status ); CHKERR_LIBXSMM_DNN( status );
    libxsmm_dnn_destroy_tensor_datalayout( libxsmm_layout );

    libxsmm_layout = libxsmm_dnn_rnncell_create_tensor_datalayout( libxsmm_handle, LIBXSMM_DNN_RNN_REGULAR_HIDDEN_STATE, &status ); CHKERR_LIBXSMM_DNN( status );
    libxsmm_hidden_state = libxsmm_dnn_link_tensor( libxsmm_layout, ht, &status ); CHKERR_LIBXSMM_DNN( status );
    libxsmm_dnn_destroy_tensor_datalayout( libxsmm_layout );

    libxsmm_layout = libxsmm_dnn_rnncell_create_tensor_datalayout( libxsmm_handle, LIBXSMM_DNN_RNN_INTERNAL_I, &status ); CHKERR_LIBXSMM_DNN( status );
    libxsmm_i = libxsmm_dnn_link_tensor( libxsmm_layout, it, &status ); CHKERR_LIBXSMM_DNN( status );
    libxsmm_dnn_destroy_tensor_datalayout( libxsmm_layout );

    libxsmm_layout = libxsmm_dnn_rnncell_create_tensor_datalayout( libxsmm_handle, LIBXSMM_DNN_RNN_INTERNAL_F, &status ); CHKERR_LIBXSMM_DNN( status );
    libxsmm_f = libxsmm_dnn_link_tensor( libxsmm_layout, ft, &status ); CHKERR_LIBXSMM_DNN( status );
    libxsmm_dnn_destroy_tensor_datalayout( libxsmm_layout );

    libxsmm_layout = libxsmm_dnn_rnncell_create_tensor_datalayout( libxsmm_handle, LIBXSMM_DNN_RNN_INTERNAL_O, &status ); CHKERR_LIBXSMM_DNN( status );
    libxsmm_o = libxsmm_dnn_link_tensor( libxsmm_layout, ot, &status ); CHKERR_LIBXSMM_DNN( status );
    libxsmm_dnn_destroy_tensor_datalayout( libxsmm_layout );

    libxsmm_layout = libxsmm_dnn_rnncell_create_tensor_datalayout( libxsmm_handle, LIBXSMM_DNN_RNN_INTERNAL_CI, &status ); CHKERR_LIBXSMM_DNN( status );
    libxsmm_ci = libxsmm_dnn_link_tensor( libxsmm_layout, cit, &status ); CHKERR_LIBXSMM_DNN( status );
    libxsmm_dnn_destroy_tensor_datalayout( libxsmm_layout );

    libxsmm_layout = libxsmm_dnn_rnncell_create_tensor_datalayout( libxsmm_handle, LIBXSMM_DNN_RNN_INTERNAL_CO, &status ); CHKERR_LIBXSMM_DNN( status );
    libxsmm_co = libxsmm_dnn_link_tensor( libxsmm_layout, cot, &status ); CHKERR_LIBXSMM_DNN( status );
    libxsmm_dnn_destroy_tensor_datalayout( libxsmm_layout );

    libxsmm_layout = libxsmm_dnn_rnncell_create_tensor_datalayout( libxsmm_handle, LIBXSMM_DNN_RNN_GRADIENT_INPUT, &status ); CHKERR_LIBXSMM_DNN( status );
    libxsmm_dinput = libxsmm_dnn_link_tensor( libxsmm_layout, dxt, &status ); CHKERR_LIBXSMM_DNN( status );
    libxsmm_dnn_destroy_tensor_datalayout( libxsmm_layout );

    libxsmm_layout = libxsmm_dnn_rnncell_create_tensor_datalayout( libxsmm_handle, LIBXSMM_DNN_RNN_GRADIENT_CS_PREV, &status ); CHKERR_LIBXSMM_DNN( status );
    libxsmm_dcs_prev = libxsmm_dnn_link_tensor( libxsmm_layout, dcsp, &status ); CHKERR_LIBXSMM_DNN( status );
    libxsmm_dnn_destroy_tensor_datalayout( libxsmm_layout );

    libxsmm_layout = libxsmm_dnn_rnncell_create_tensor_datalayout( libxsmm_handle, LIBXSMM_DNN_RNN_GRADIENT_HIDDEN_STATE_PREV, &status ); CHKERR_LIBXSMM_DNN( status );
    libxsmm_dhidden_state_prev = libxsmm_dnn_link_tensor( libxsmm_layout, dhp, &status ); CHKERR_LIBXSMM_DNN( status );
    libxsmm_dnn_destroy_tensor_datalayout( libxsmm_layout );

    libxsmm_layout = libxsmm_dnn_rnncell_create_tensor_datalayout( libxsmm_handle, LIBXSMM_DNN_RNN_GRADIENT_WEIGHT, &status ); CHKERR_LIBXSMM_DNN( status );
    libxsmm_dweight = libxsmm_dnn_link_tensor( libxsmm_layout, dw, &status ); CHKERR_LIBXSMM_DNN( status );
    libxsmm_dnn_destroy_tensor_datalayout( libxsmm_layout );

    libxsmm_layout = libxsmm_dnn_rnncell_create_tensor_datalayout( libxsmm_handle, LIBXSMM_DNN_RNN_GRADIENT_RECUR_WEIGHT, &status ); CHKERR_LIBXSMM_DNN( status );
    libxsmm_drecur_weight = libxsmm_dnn_link_tensor( libxsmm_layout, dr, &status ); CHKERR_LIBXSMM_DNN( status );
    libxsmm_dnn_destroy_tensor_datalayout( libxsmm_layout );

    libxsmm_layout = libxsmm_dnn_rnncell_create_tensor_datalayout( libxsmm_handle, LIBXSMM_DNN_RNN_GRADIENT_BIAS, &status ); CHKERR_LIBXSMM_DNN( status );
    libxsmm_dbias = libxsmm_dnn_link_tensor( libxsmm_layout, db, &status ); CHKERR_LIBXSMM_DNN( status );
    libxsmm_dnn_destroy_tensor_datalayout( libxsmm_layout );

    libxsmm_layout = libxsmm_dnn_rnncell_create_tensor_datalayout( libxsmm_handle, LIBXSMM_DNN_RNN_GRADIENT_CS, &status ); CHKERR_LIBXSMM_DNN( status );
    libxsmm_dcs = libxsmm_dnn_link_tensor( libxsmm_layout, dcs, &status ); CHKERR_LIBXSMM_DNN( status );
    libxsmm_dnn_destroy_tensor_datalayout( libxsmm_layout );

    libxsmm_layout = libxsmm_dnn_rnncell_create_tensor_datalayout( libxsmm_handle, LIBXSMM_DNN_RNN_GRADIENT_HIDDEN_STATE, &status ); CHKERR_LIBXSMM_DNN( status );
    libxsmm_dhidden_state = libxsmm_dnn_link_tensor( libxsmm_layout, dht, &status ); CHKERR_LIBXSMM_DNN( status );
    libxsmm_dnn_destroy_tensor_datalayout( libxsmm_layout );

    /* copy in data to LIBXSMM format */
    matrix_copy(N*C*t, xgoldt, xt);
    matrix_copy(K*N, cspgold, csp);
    matrix_copy(K*N, hpgold, hp);
    convert_ck_c4k(C, K, wigold, w);
    convert_ck_c4k(C, K, wcgold, &(w[K]));
    convert_ck_c4k(C, K, wfgold, &(w[2*K]));
    convert_ck_c4k(C, K, wogold, &(w[3*K]));
    convert_ck_c4k(K, K, rigold, r);
    convert_ck_c4k(K, K, rcgold, &(r[K]));
    convert_ck_c4k(K, K, rfgold, &(r[2*K]));
    convert_ck_c4k(K, K, rogold, &(r[3*K]));
    matrix_copy(K, bigold, &(b[0]));
    matrix_copy(K, bcgold, &(b[K]));
    matrix_copy(K, bfgold, &(b[2*K]));
    matrix_copy(K, bogold, &(b[3*K]));
    matrix_copy(K*N*t, dhgoldt, dht);
    matrix_copy(K*N, dcsgold, dcs);

    /* bind buffers and filter to handle */
    CHKERR_LIBXSMM_DNN( libxsmm_dnn_rnncell_bind_tensor( libxsmm_handle, libxsmm_input, LIBXSMM_DNN_RNN_REGULAR_INPUT ) );
    CHKERR_LIBXSMM_DNN( libxsmm_dnn_rnncell_bind_tensor( libxsmm_handle, libxsmm_cs_prev, LIBXSMM_DNN_RNN_REGULAR_CS_PREV ) );
    CHKERR_LIBXSMM_DNN( libxsmm_dnn_rnncell_bind_tensor( libxsmm_handle, libxsmm_hidden_state_prev, LIBXSMM_DNN_RNN_REGULAR_HIDDEN_STATE_PREV ) );
    CHKERR_LIBXSMM_DNN( libxsmm_dnn_rnncell_bind_tensor( libxsmm_handle, libxsmm_weight, LIBXSMM_DNN_RNN_REGULAR_WEIGHT ) );
    CHKERR_LIBXSMM_DNN( libxsmm_dnn_rnncell_bind_tensor( libxsmm_handle, libxsmm_recur_weight, LIBXSMM_DNN_RNN_REGULAR_RECUR_WEIGHT ) );
    CHKERR_LIBXSMM_DNN( libxsmm_dnn_rnncell_bind_tensor( libxsmm_handle, libxsmm_bias, LIBXSMM_DNN_RNN_REGULAR_BIAS ) );
    CHKERR_LIBXSMM_DNN( libxsmm_dnn_rnncell_bind_tensor( libxsmm_handle, libxsmm_cs, LIBXSMM_DNN_RNN_REGULAR_CS ) );
    CHKERR_LIBXSMM_DNN( libxsmm_dnn_rnncell_bind_tensor( libxsmm_handle, libxsmm_hidden_state, LIBXSMM_DNN_RNN_REGULAR_HIDDEN_STATE ) );
    CHKERR_LIBXSMM_DNN( libxsmm_dnn_rnncell_bind_tensor( libxsmm_handle, libxsmm_i, LIBXSMM_DNN_RNN_INTERNAL_I ) );
    CHKERR_LIBXSMM_DNN( libxsmm_dnn_rnncell_bind_tensor( libxsmm_handle, libxsmm_f, LIBXSMM_DNN_RNN_INTERNAL_F ) );
    CHKERR_LIBXSMM_DNN( libxsmm_dnn_rnncell_bind_tensor( libxsmm_handle, libxsmm_o, LIBXSMM_DNN_RNN_INTERNAL_O ) );
    CHKERR_LIBXSMM_DNN( libxsmm_dnn_rnncell_bind_tensor( libxsmm_handle, libxsmm_ci, LIBXSMM_DNN_RNN_INTERNAL_CI ) );
    CHKERR_LIBXSMM_DNN( libxsmm_dnn_rnncell_bind_tensor( libxsmm_handle, libxsmm_co, LIBXSMM_DNN_RNN_INTERNAL_CO ) );
    CHKERR_LIBXSMM_DNN( libxsmm_dnn_rnncell_bind_tensor( libxsmm_handle, libxsmm_dinput, LIBXSMM_DNN_RNN_GRADIENT_INPUT ) );
    CHKERR_LIBXSMM_DNN( libxsmm_dnn_rnncell_bind_tensor( libxsmm_handle, libxsmm_dcs_prev, LIBXSMM_DNN_RNN_GRADIENT_CS_PREV ) );
    CHKERR_LIBXSMM_DNN( libxsmm_dnn_rnncell_bind_tensor( libxsmm_handle, libxsmm_dhidden_state_prev, LIBXSMM_DNN_RNN_GRADIENT_HIDDEN_STATE_PREV ) );
    CHKERR_LIBXSMM_DNN( libxsmm_dnn_rnncell_bind_tensor( libxsmm_handle, libxsmm_dweight, LIBXSMM_DNN_RNN_GRADIENT_WEIGHT ) );
    CHKERR_LIBXSMM_DNN( libxsmm_dnn_rnncell_bind_tensor( libxsmm_handle, libxsmm_drecur_weight, LIBXSMM_DNN_RNN_GRADIENT_RECUR_WEIGHT ) );
    CHKERR_LIBXSMM_DNN( libxsmm_dnn_rnncell_bind_tensor( libxsmm_handle, libxsmm_dbias, LIBXSMM_DNN_RNN_GRADIENT_BIAS ) );
    CHKERR_LIBXSMM_DNN( libxsmm_dnn_rnncell_bind_tensor( libxsmm_handle, libxsmm_dcs, LIBXSMM_DNN_RNN_GRADIENT_CS ) );
    CHKERR_LIBXSMM_DNN( libxsmm_dnn_rnncell_bind_tensor( libxsmm_handle, libxsmm_dhidden_state, LIBXSMM_DNN_RNN_GRADIENT_HIDDEN_STATE ) );

    /* let's allocate and bind scratch */
    if (pass == 0) {
      scratch_size = libxsmm_dnn_rnncell_get_scratch_size( libxsmm_handle, LIBXSMM_DNN_COMPUTE_KIND_FWD, &status );
      CHKERR_LIBXSMM_DNN( status );
      scratch = libxsmm_aligned_malloc( scratch_size, 2097152 );
      CHKERR_LIBXSMM_DNN( libxsmm_dnn_rnncell_bind_scratch( libxsmm_handle, LIBXSMM_DNN_COMPUTE_KIND_FWD, scratch ) );
    } else {
      scratch_size = libxsmm_dnn_rnncell_get_scratch_size( libxsmm_handle, LIBXSMM_DNN_COMPUTE_KIND_ALL, &status );
      CHKERR_LIBXSMM_DNN( status );
      scratch = libxsmm_aligned_malloc( scratch_size, 2097152 );
      CHKERR_LIBXSMM_DNN( libxsmm_dnn_rnncell_bind_scratch( libxsmm_handle, LIBXSMM_DNN_COMPUTE_KIND_ALL, scratch ) );
    }
    zero_buf( (float*)scratch, scratch_size/4 );

    /* let's allocate and bind internalstate */
    if (pass == 0) {
      internalstate_size = libxsmm_dnn_rnncell_get_internalstate_size( libxsmm_handle, LIBXSMM_DNN_COMPUTE_KIND_FWD, &status );
      CHKERR_LIBXSMM_DNN( status );
      internalstate = (0 != internalstate_size ? libxsmm_aligned_malloc( internalstate_size, 2097152 ) : NULL);
      CHKERR_LIBXSMM_DNN( libxsmm_dnn_rnncell_bind_internalstate( libxsmm_handle, LIBXSMM_DNN_COMPUTE_KIND_FWD, internalstate ) );
    } else {
      internalstate_size = libxsmm_dnn_rnncell_get_internalstate_size( libxsmm_handle, LIBXSMM_DNN_COMPUTE_KIND_ALL, &status );
      CHKERR_LIBXSMM_DNN( status );
      internalstate = (0 != internalstate_size ? libxsmm_aligned_malloc( internalstate_size, 2097152 ) : NULL);
      CHKERR_LIBXSMM_DNN( libxsmm_dnn_rnncell_bind_internalstate( libxsmm_handle, LIBXSMM_DNN_COMPUTE_KIND_ALL, internalstate ) );
    }
    zero_buf( (float*)internalstate, internalstate_size/4 );

    if ((pass == 0) && LIBXSMM_NEQ(0, check)) {
      printf("##########################################\n");
      printf("#   Correctness - FWD (custom-Storage)   #\n");
      printf("##########################################\n");
      /* run LIBXSMM LSTM */
#if defined(_OPENMP)
#     pragma omp parallel
#endif
      {
#if defined(_OPENMP)
        const int tid = omp_get_thread_num();
#else
        const int tid = 0;
#endif
        CHKERR_LIBXSMM_DNN( libxsmm_dnn_rnncell_execute_st( libxsmm_handle, LIBXSMM_DNN_COMPUTE_KIND_FWD, 0, tid ) );
      }

      /* compare */
      libxsmm_matdiff(&norms_fwd, LIBXSMM_DATATYPE_F32, K*N, 1, &LIBXSMM_VLA_ACCESS(2, hgold, t-1, 0, K * N), &LIBXSMM_VLA_ACCESS(2, h, t-1, 0, K * N), 0, 0);
      printf("L1 reference  : %.25g\n", norms_fwd.l1_ref);
      printf("L1 test       : %.25g\n", norms_fwd.l1_tst);
      printf("L2 abs.error  : %.24f\n", norms_fwd.l2_abs);
      printf("L2 rel.error  : %.24f\n", norms_fwd.l2_rel);
      printf("Linf abs.error: %.24f\n", norms_fwd.linf_abs);
      printf("Linf rel.error: %.24f\n", norms_fwd.linf_rel);
      printf("Check-norm    : %.24f\n", norms_fwd.normf_rel);
      libxsmm_matdiff_reduce(&diff, &norms_fwd);
    } else {
      /* We need to always run FWD pass once to populate i, f, o, ci, co, cs, h */
#if defined(_OPENMP)
#     pragma omp parallel
#endif
      {
#if defined(_OPENMP)
        const int tid = omp_get_thread_num();
#else
        const int tid = 0;
#endif
        CHKERR_LIBXSMM_DNN( libxsmm_dnn_rnncell_execute_st( libxsmm_handle, LIBXSMM_DNN_COMPUTE_KIND_FWD, 0, tid ) );
      }
    }

    if ( (pass == 1) && LIBXSMM_NEQ(0, check) ) {
      printf("##########################################\n");
      printf("#   Correctness - BWD (custom-Storage)   #\n");
      printf("##########################################\n");
      /* run LIBXSMM LSTM */
#if defined(_OPENMP)
#     pragma omp parallel
#endif
      {
#if defined(_OPENMP)
        const int tid = omp_get_thread_num();
#else
        const int tid = 0;
#endif
        CHKERR_LIBXSMM_DNN( libxsmm_dnn_rnncell_execute_st( libxsmm_handle, LIBXSMM_DNN_COMPUTE_KIND_BWD, 0, tid ) );
      }

      /* compare */
      libxsmm_matdiff(&norms_bwd, LIBXSMM_DATATYPE_F32, N*C*t, 1, dxgoldt, dxt, 0, 0);
      printf("L1 reference  : %.25g\n", norms_bwd.l1_ref);
      printf("L1 test       : %.25g\n", norms_bwd.l1_tst);
      printf("L2 abs.error  : %.24f\n", norms_bwd.l2_abs);
      printf("L2 rel.error  : %.24f\n", norms_bwd.l2_rel);
      printf("Linf abs.error: %.24f\n", norms_bwd.linf_abs);
      printf("Linf rel.error: %.24f\n", norms_bwd.linf_rel);
      printf("Check-norm    : %.24f\n", norms_bwd.normf_rel);
      libxsmm_matdiff_reduce(&diff, &norms_bwd);
    }

    if ( (pass == 2) && LIBXSMM_NEQ(0, check) ) {
      printf("##########################################\n");
      printf("#   Correctness - UPD (custom-Storage)   #\n");
      printf("##########################################\n");
      /* run LIBXSMM LSTM */
#if defined(_OPENMP)
#     pragma omp parallel
#endif
      {
#if defined(_OPENMP)
        const int tid = omp_get_thread_num();
#else
        const int tid = 0;
#endif
        CHKERR_LIBXSMM_DNN( libxsmm_dnn_rnncell_execute_st( libxsmm_handle, LIBXSMM_DNN_COMPUTE_KIND_UPD, 0, tid ) );
      }

      /* compare */
      libxsmm_matdiff(&norms_upd_w, LIBXSMM_DATATYPE_F32, C*K*4, 1, dwgold, dw, 0, 0);
      printf("Delta weight\n");
      printf("L1 reference  : %.25g\n", norms_upd_w.l1_ref);
      printf("L1 test       : %.25g\n", norms_upd_w.l1_tst);
      printf("L2 abs.error  : %.24f\n", norms_upd_w.l2_abs);
      printf("L2 rel.error  : %.24f\n", norms_upd_w.l2_rel);
      printf("Linf abs.error: %.24f\n", norms_upd_w.linf_abs);
      printf("Linf rel.error: %.24f\n", norms_upd_w.linf_rel);
      printf("Check-norm    : %.24f\n", norms_upd_w.normf_rel);
      libxsmm_matdiff_reduce(&diff, &norms_upd_w);

#if defined(TWO_GEMMS)
      libxsmm_matdiff(&norms_upd_r, LIBXSMM_DATATYPE_F32, K*K*4, 1, drgold, dr, 0, 0);
#else
      libxsmm_matdiff(&norms_upd_r, LIBXSMM_DATATYPE_F32, K*K*4, 1, &(dwgold[C*K*4]), dr, 0, 0);
#endif
      printf("Delta recurrent weight\n");
      printf("L1 reference  : %.25g\n", norms_upd_r.l1_ref);
      printf("L1 test       : %.25g\n", norms_upd_r.l1_tst);
      printf("L2 abs.error  : %.24f\n", norms_upd_r.l2_abs);
      printf("L2 rel.error  : %.24f\n", norms_upd_r.l2_rel);
      printf("Linf abs.error: %.24f\n", norms_upd_r.linf_abs);
      printf("Linf rel.error: %.24f\n", norms_upd_r.linf_rel);
      printf("Check-norm    : %.24f\n", norms_upd_r.normf_rel);
      libxsmm_matdiff_reduce(&diff, &norms_upd_r);

      libxsmm_matdiff(&norms_upd_b, LIBXSMM_DATATYPE_F32, K*4, 1, dbgold, db, 0, 0);
      printf("Delta bias\n");
      printf("L1 reference  : %.25g\n", norms_upd_b.l1_ref);
      printf("L1 test       : %.25g\n", norms_upd_b.l1_tst);
      printf("L2 abs.error  : %.24f\n", norms_upd_b.l2_abs);
      printf("L2 rel.error  : %.24f\n", norms_upd_b.l2_rel);
      printf("Linf abs.error: %.24f\n", norms_upd_b.linf_abs);
      printf("Linf rel.error: %.24f\n", norms_upd_b.linf_rel);
      printf("Check-norm    : %.24f\n", norms_upd_b.normf_rel);
      libxsmm_matdiff_reduce(&diff, &norms_upd_b);
    }

    if ( (pass == 3) && LIBXSMM_NEQ(0, check) ) {
      printf("##########################################\n");
      printf("# Correctness - BWD+UPD (custom-Storage) #\n");
      printf("##########################################\n");
      /* run LIBXSMM LSTM */
#if defined(_OPENMP)
#     pragma omp parallel
#endif
      {
#if defined(_OPENMP)
        const int tid = omp_get_thread_num();
#else
        const int tid = 0;
#endif
        CHKERR_LIBXSMM_DNN( libxsmm_dnn_rnncell_execute_st( libxsmm_handle, LIBXSMM_DNN_COMPUTE_KIND_BWDUPD, 0, tid ) );
      }

      /* compare */
      libxsmm_matdiff(&norms_bwd, LIBXSMM_DATATYPE_F32, N*C*t, 1, dxgoldt, dxt, 0, 0);
      printf("Delta input\n");
      printf("L1 reference  : %.25g\n", norms_bwd.l1_ref);
      printf("L1 test       : %.25g\n", norms_bwd.l1_tst);
      printf("L2 abs.error  : %.24f\n", norms_bwd.l2_abs);
      printf("L2 rel.error  : %.24f\n", norms_bwd.l2_rel);
      printf("Linf abs.error: %.24f\n", norms_bwd.linf_abs);
      printf("Linf rel.error: %.24f\n", norms_bwd.linf_rel);
      printf("Check-norm    : %.24f\n", norms_bwd.normf_rel);
      libxsmm_matdiff_reduce(&diff, &norms_bwd);

      libxsmm_matdiff(&norms_upd_w, LIBXSMM_DATATYPE_F32, C*K*4, 1, dwgold, dw, 0, 0);
      printf("Delta weight\n");
      printf("L1 reference  : %.25g\n", norms_upd_w.l1_ref);
      printf("L1 test       : %.25g\n", norms_upd_w.l1_tst);
      printf("L2 abs.error  : %.24f\n", norms_upd_w.l2_abs);
      printf("L2 rel.error  : %.24f\n", norms_upd_w.l2_rel);
      printf("Linf abs.error: %.24f\n", norms_upd_w.linf_abs);
      printf("Linf rel.error: %.24f\n", norms_upd_w.linf_rel);
      printf("Check-norm    : %.24f\n", norms_upd_w.normf_rel);
      libxsmm_matdiff_reduce(&diff, &norms_upd_w);

#if defined(TWO_GEMMS)
      libxsmm_matdiff(&norms_upd_r, LIBXSMM_DATATYPE_F32, K*K*4, 1, drgold, dr, 0, 0);
#else
      libxsmm_matdiff(&norms_upd_r, LIBXSMM_DATATYPE_F32, K*K*4, 1, &(dwgold[C*K*4]), dr, 0, 0);
#endif
      printf("Delta recurrent weight\n");
      printf("L1 reference  : %.25g\n", norms_upd_r.l1_ref);
      printf("L1 test       : %.25g\n", norms_upd_r.l1_tst);
      printf("L2 abs.error  : %.24f\n", norms_upd_r.l2_abs);
      printf("L2 rel.error  : %.24f\n", norms_upd_r.l2_rel);
      printf("Linf abs.error: %.24f\n", norms_upd_r.linf_abs);
      printf("Linf rel.error: %.24f\n", norms_upd_r.linf_rel);
      printf("Check-norm    : %.24f\n", norms_upd_r.normf_rel);
      libxsmm_matdiff_reduce(&diff, &norms_upd_r);

      libxsmm_matdiff(&norms_upd_b, LIBXSMM_DATATYPE_F32, K*4, 1, dbgold, db, 0, 0);
      printf("Delta bias\n");
      printf("L1 reference  : %.25g\n", norms_upd_b.l1_ref);
      printf("L1 test       : %.25g\n", norms_upd_b.l1_tst);
      printf("L2 abs.error  : %.24f\n", norms_upd_b.l2_abs);
      printf("L2 rel.error  : %.24f\n", norms_upd_b.l2_rel);
      printf("Linf abs.error: %.24f\n", norms_upd_b.linf_abs);
      printf("Linf rel.error: %.24f\n", norms_upd_b.linf_rel);
      printf("Check-norm    : %.24f\n", norms_upd_b.normf_rel);
      libxsmm_matdiff_reduce(&diff, &norms_upd_b);
    }

    if ( pass == 0 ) {
      printf("##########################################\n");
      printf("#   Performance - FWD (custom-Storage)   #\n");
      printf("##########################################\n");
      /* run LIBXSMM LSTM for performance */
      l_start = libxsmm_timer_tick();

#if defined(_OPENMP)
#     pragma omp parallel private(j)
#endif
      {
#if defined(_OPENMP)
        const int tid = omp_get_thread_num();
#else
        const int tid = 0;
#endif
        for (j = 0; j < iters; ++j) {
          libxsmm_dnn_rnncell_execute_st( libxsmm_handle, LIBXSMM_DNN_COMPUTE_KIND_FWD, 0, tid );
        }
      }
      l_end = libxsmm_timer_tick();
      l_total = libxsmm_timer_duration(l_start, l_end);
      flops = (((2.0 * K * N * C) + (2.0 * K * N * K) + (2.0 * K * N) + (tflops * K * N)) * 4.0 + (4.0 * K * N) + (tflops * K * N)) * (double)t * (double)iters;

      printf("GFLOP  = %.5g\n", flops*1e-9/(double)iters);
      printf("fp time = %.5g\n", ((double)(l_total/iters)));
      printf("GFLOPS  = %.5g\n", (flops*1e-9)/l_total);

      printf("PERFDUMP,FP,%s,%i,%i,%i,%i,%i,%.5g,%.5g\n", LIBXSMM_VERSION, nThreads, N, C, K, t, ((double)(l_total/iters)), (flops*1e-9)/l_total);
    }

    if ( pass == 1 ) {
      printf("##########################################\n");
      printf("#   Performance - BWD (custom-Storage)   #\n");
      printf("##########################################\n");
      /* run LIBXSMM LSTM for performance */
      l_start = libxsmm_timer_tick();

#if defined(_OPENMP)
#     pragma omp parallel private(j)
#endif
      {
#if defined(_OPENMP)
        const int tid = omp_get_thread_num();
#else
        const int tid = 0;
#endif
        for (j = 0; j < iters; ++j) {
          libxsmm_dnn_rnncell_execute_st( libxsmm_handle, LIBXSMM_DNN_COMPUTE_KIND_BWD, 0, tid );
        }
      }
      l_end = libxsmm_timer_tick();
      l_total = libxsmm_timer_duration(l_start, l_end);
      flops = K * N; /* delta + delta_out */
      flops += (6.0 * K * N + tflops * K * N); /* dJdd */
      flops += (4.0 * K * N); /* dJdc */
      flops += (4.0 * K * N); /* dJdi */
      flops += (4.0 * K * N); /* dJdf */
      flops += (4.0 * K * N + tflops * K * N); /* dJdo */
      tempflops = (4.0 * K * C); /* W^T */
      tempflops += (8.0 * K * N * C); /* W^T * dJd{c, i, f, o} */
      tempflops += (3.0 * K * C); /* summation */
      flops += tempflops;
      tempflops = (4.0 * K * K); /* R^T */
      tempflops += (8.0 * K * N * K); /* R^T * dJd{c, i, f, o} */
      flops += tempflops;
      flops *= t; /* for t time steps */
      flops *= iters;

      printf("GFLOP  = %.5g\n", flops*1e-9/(double)iters);
      printf("bp time = %.5g\n", ((double)(l_total/iters)));
      printf("GFLOPS  = %.5g\n", (flops*1e-9)/l_total);

      printf("PERFDUMP,BP,%s,%i,%i,%i,%i,%i,%.5g,%.5g\n", LIBXSMM_VERSION, nThreads, N, C, K, t, ((double)(l_total/iters)), (flops*1e-9)/l_total);
    }

    if ( pass == 2 ) {
      printf("##########################################\n");
      printf("#   Performance - UPD (custom-Storage)   #\n");
      printf("##########################################\n");
      /* run LIBXSMM LSTM for performance */
      l_start = libxsmm_timer_tick();

#if defined(_OPENMP)
#     pragma omp parallel private(j)
#endif
      {
#if defined(_OPENMP)
        const int tid = omp_get_thread_num();
#else
        const int tid = 0;
#endif
        for (j = 0; j < iters; ++j) {
          libxsmm_dnn_rnncell_execute_st( libxsmm_handle, LIBXSMM_DNN_COMPUTE_KIND_UPD, 0, tid );
        }
      }
      l_end = libxsmm_timer_tick();
      l_total = libxsmm_timer_duration(l_start, l_end);
      flops = K * N; /* delta + delta_out */
      flops += (6.0 * K * N + tflops * K * N); /* dJdd */
      flops += (4.0 * K * N); /* dJdc */
      flops += (4.0 * K * N); /* dJdi */
      flops += (4.0 * K * N); /* dJdf */
      flops += (4.0 * K * N + tflops * K * N); /* dJdo */
      tempflops = (4.0 * K * K); /* R^T */
      tempflops += (8.0 * K * N * K); /* R^T * dJd{c, i, f, o} */
      flops += tempflops;
      flops *= t; /* for t time steps */
      tempflops = C * N; /* x^T */
      tempflops += (8.0 * K * N * C); /* delta{c, i, f, o} * x^T */
      tempflops *= t; /* for t time steps */
      tempflops += (4.0 * K * C * (t-1)); /* for summation of dJdW{c, i, f, o} */
      flops += tempflops;
      tempflops = 4.0 * K * N; /* delta^T */
      tempflops += (8.0 * K * N * K); /* delta{c, i, f, o} * delta^T */
      tempflops *= (t - 1); /* for (t - 1) time steps */
      tempflops += (4.0 * K * N * (t-2)); /* for summation of dJdR{c, i, f, o} */
      flops += tempflops;
      flops += (4.0 * K * N * (t - 1)); /* delbias */
      flops *= iters;

      printf("GFLOP  = %.5g\n", flops*1e-9/(double)iters);
      printf("wu time = %.5g\n", ((double)(l_total/iters)));
      printf("GFLOPS  = %.5g\n", (flops*1e-9)/l_total);

      printf("PERFDUMP,WU,%s,%i,%i,%i,%i,%i,%.5g,%.5g\n", LIBXSMM_VERSION, nThreads, N, C, K, t, ((double)(l_total/iters)), (flops*1e-9)/l_total);
    }

    if ( pass == 3 ) {
      printf("##########################################\n");
      printf("# Performance - BWD+UPD (custom-Storage) #\n");
      printf("##########################################\n");
      /* run LIBXSMM LSTM for performance */
      l_start = libxsmm_timer_tick();

#if defined(_OPENMP)
#     pragma omp parallel private(j)
#endif
      {
#if defined(_OPENMP)
        const int tid = omp_get_thread_num();
#else
        const int tid = 0;
#endif
        for (j = 0; j < iters; ++j) {
          libxsmm_dnn_rnncell_execute_st( libxsmm_handle, LIBXSMM_DNN_COMPUTE_KIND_BWDUPD, 0, tid );
        }
      }
      l_end = libxsmm_timer_tick();
      l_total = libxsmm_timer_duration(l_start, l_end);
      flops = K * N; /* delta + delta_out */
      flops += (6.0 * K * N + tflops * K * N); /* dJdd */
      flops += (4.0 * K * N); /* dJdc */
      flops += (4.0 * K * N); /* dJdi */
      flops += (4.0 * K * N); /* dJdf */
      flops += (4.0 * K * N + tflops * K * N); /* dJdo */
      tempflops = (4.0 * K * C); /* W^T */
      tempflops += (8.0 * K * N * C); /* W^T * dJd{c, i, f, o} */
      tempflops += (3.0 * K * C); /* summation */
      flops += tempflops;
      tempflops = (4.0 * K * K); /* R^T */
      tempflops += (8.0 * K * N * K); /* R^T * dJd{c, i, f, o} */
      flops += tempflops;
      flops *= t; /* for t time steps */
      tempflops = C * N; /* x^T */
      tempflops += (8.0 * K * N * C); /* delta{c, i, f, o} * x^T */
      tempflops *= t; /* for t time steps */
      tempflops += (4.0 * K * C * (t-1)); /* for summation of dJdW{c, i, f, o} */
      flops += tempflops;
      tempflops = 4.0 * K * N; /* delta^T */
      tempflops += (8.0 * K * N * K); /* delta{c, i, f, o} * delta^T */
      tempflops *= (t - 1); /* for (t - 1) time steps */
      tempflops += (4.0 * K * N * (t-2)); /* for summation of dJdR{c, i, f, o} */
      flops += tempflops;
      flops += (4.0 * K * N * (t - 1)); /* delbias */
      flops *= iters;

      printf("GFLOP  = %.5g\n", flops*1e-9/(double)iters);
      printf("bp+wu time = %.5g\n", ((double)(l_total/iters)));
      printf("GFLOPS  = %.5g\n", (flops*1e-9)/l_total);

      printf("PERFDUMP,BP+WU,%s,%i,%i,%i,%i,%i,%.5g,%.5g\n", LIBXSMM_VERSION, nThreads, N, C, K, t, ((double)(l_total/iters)), (flops*1e-9)/l_total);
    }

    /* clean-up */
    if (pass == 0) {
      CHKERR_LIBXSMM_DNN( libxsmm_dnn_rnncell_release_scratch( libxsmm_handle, LIBXSMM_DNN_COMPUTE_KIND_FWD ) );
      CHKERR_LIBXSMM_DNN( libxsmm_dnn_rnncell_release_internalstate( libxsmm_handle, LIBXSMM_DNN_COMPUTE_KIND_FWD ) );
    } else {
      CHKERR_LIBXSMM_DNN( libxsmm_dnn_rnncell_release_scratch( libxsmm_handle, LIBXSMM_DNN_COMPUTE_KIND_ALL ) );
      CHKERR_LIBXSMM_DNN( libxsmm_dnn_rnncell_release_internalstate( libxsmm_handle, LIBXSMM_DNN_COMPUTE_KIND_ALL ) );
    }
    libxsmm_free(scratch);
    libxsmm_free(internalstate);
    CHKERR_LIBXSMM_DNN( libxsmm_dnn_rnncell_release_tensor( libxsmm_handle, LIBXSMM_DNN_RNN_REGULAR_INPUT ) );
    CHKERR_LIBXSMM_DNN( libxsmm_dnn_rnncell_release_tensor( libxsmm_handle, LIBXSMM_DNN_RNN_REGULAR_CS_PREV ) );
    CHKERR_LIBXSMM_DNN( libxsmm_dnn_rnncell_release_tensor( libxsmm_handle, LIBXSMM_DNN_RNN_REGULAR_HIDDEN_STATE_PREV ) );
    CHKERR_LIBXSMM_DNN( libxsmm_dnn_rnncell_release_tensor( libxsmm_handle, LIBXSMM_DNN_RNN_REGULAR_WEIGHT ) );
    CHKERR_LIBXSMM_DNN( libxsmm_dnn_rnncell_release_tensor( libxsmm_handle, LIBXSMM_DNN_RNN_REGULAR_RECUR_WEIGHT ) );
    CHKERR_LIBXSMM_DNN( libxsmm_dnn_rnncell_release_tensor( libxsmm_handle, LIBXSMM_DNN_RNN_REGULAR_BIAS ) );
    CHKERR_LIBXSMM_DNN( libxsmm_dnn_rnncell_release_tensor( libxsmm_handle, LIBXSMM_DNN_RNN_REGULAR_CS ) );
    CHKERR_LIBXSMM_DNN( libxsmm_dnn_rnncell_release_tensor( libxsmm_handle, LIBXSMM_DNN_RNN_REGULAR_HIDDEN_STATE ) );
    CHKERR_LIBXSMM_DNN( libxsmm_dnn_rnncell_release_tensor( libxsmm_handle, LIBXSMM_DNN_RNN_INTERNAL_I ) );
    CHKERR_LIBXSMM_DNN( libxsmm_dnn_rnncell_release_tensor( libxsmm_handle, LIBXSMM_DNN_RNN_INTERNAL_F ) );
    CHKERR_LIBXSMM_DNN( libxsmm_dnn_rnncell_release_tensor( libxsmm_handle, LIBXSMM_DNN_RNN_INTERNAL_O ) );
    CHKERR_LIBXSMM_DNN( libxsmm_dnn_rnncell_release_tensor( libxsmm_handle, LIBXSMM_DNN_RNN_INTERNAL_CI ) );
    CHKERR_LIBXSMM_DNN( libxsmm_dnn_rnncell_release_tensor( libxsmm_handle, LIBXSMM_DNN_RNN_INTERNAL_CO ) );
    CHKERR_LIBXSMM_DNN( libxsmm_dnn_rnncell_release_tensor( libxsmm_handle, LIBXSMM_DNN_RNN_GRADIENT_INPUT ) );
    CHKERR_LIBXSMM_DNN( libxsmm_dnn_rnncell_release_tensor( libxsmm_handle, LIBXSMM_DNN_RNN_GRADIENT_CS_PREV ) );
    CHKERR_LIBXSMM_DNN( libxsmm_dnn_rnncell_release_tensor( libxsmm_handle, LIBXSMM_DNN_RNN_GRADIENT_HIDDEN_STATE_PREV ) );
    CHKERR_LIBXSMM_DNN( libxsmm_dnn_rnncell_release_tensor( libxsmm_handle, LIBXSMM_DNN_RNN_GRADIENT_WEIGHT ) );
    CHKERR_LIBXSMM_DNN( libxsmm_dnn_rnncell_release_tensor( libxsmm_handle, LIBXSMM_DNN_RNN_GRADIENT_RECUR_WEIGHT ) );
    CHKERR_LIBXSMM_DNN( libxsmm_dnn_rnncell_release_tensor( libxsmm_handle, LIBXSMM_DNN_RNN_GRADIENT_BIAS ) );
    CHKERR_LIBXSMM_DNN( libxsmm_dnn_rnncell_release_tensor( libxsmm_handle, LIBXSMM_DNN_RNN_GRADIENT_CS ) );
    CHKERR_LIBXSMM_DNN( libxsmm_dnn_rnncell_release_tensor( libxsmm_handle, LIBXSMM_DNN_RNN_GRADIENT_HIDDEN_STATE ) );
    CHKERR_LIBXSMM_DNN( libxsmm_dnn_destroy_tensor( libxsmm_input ) );
    CHKERR_LIBXSMM_DNN( libxsmm_dnn_destroy_tensor( libxsmm_cs_prev ) );
    CHKERR_LIBXSMM_DNN( libxsmm_dnn_destroy_tensor( libxsmm_hidden_state_prev ) );
    CHKERR_LIBXSMM_DNN( libxsmm_dnn_destroy_tensor( libxsmm_weight ) );
    CHKERR_LIBXSMM_DNN( libxsmm_dnn_destroy_tensor( libxsmm_recur_weight ) );
    CHKERR_LIBXSMM_DNN( libxsmm_dnn_destroy_tensor( libxsmm_bias ) );
    CHKERR_LIBXSMM_DNN( libxsmm_dnn_destroy_tensor( libxsmm_cs ) );
    CHKERR_LIBXSMM_DNN( libxsmm_dnn_destroy_tensor( libxsmm_hidden_state ) );
    CHKERR_LIBXSMM_DNN( libxsmm_dnn_destroy_tensor( libxsmm_i ) );
    CHKERR_LIBXSMM_DNN( libxsmm_dnn_destroy_tensor( libxsmm_f ) );
    CHKERR_LIBXSMM_DNN( libxsmm_dnn_destroy_tensor( libxsmm_o ) );
    CHKERR_LIBXSMM_DNN( libxsmm_dnn_destroy_tensor( libxsmm_ci ) );
    CHKERR_LIBXSMM_DNN( libxsmm_dnn_destroy_tensor( libxsmm_co ) );
    CHKERR_LIBXSMM_DNN( libxsmm_dnn_destroy_tensor( libxsmm_dinput ) );
    CHKERR_LIBXSMM_DNN( libxsmm_dnn_destroy_tensor( libxsmm_dcs_prev ) );
    CHKERR_LIBXSMM_DNN( libxsmm_dnn_destroy_tensor( libxsmm_dhidden_state_prev ) );
    CHKERR_LIBXSMM_DNN( libxsmm_dnn_destroy_tensor( libxsmm_dweight ) );
    CHKERR_LIBXSMM_DNN( libxsmm_dnn_destroy_tensor( libxsmm_drecur_weight ) );
    CHKERR_LIBXSMM_DNN( libxsmm_dnn_destroy_tensor( libxsmm_dbias ) );
    CHKERR_LIBXSMM_DNN( libxsmm_dnn_destroy_tensor( libxsmm_dcs ) );
    CHKERR_LIBXSMM_DNN( libxsmm_dnn_destroy_tensor( libxsmm_dhidden_state ) );
    CHKERR_LIBXSMM_DNN( libxsmm_dnn_destroy_rnncell( libxsmm_handle ) );
  }

  /* deallocate data */
  libxsmm_free(xgoldt);
  libxsmm_free(cspgold);
  libxsmm_free(hpgold);
  libxsmm_free(wigold);
  libxsmm_free(wfgold);
  libxsmm_free(wogold);
  libxsmm_free(wcgold);
  libxsmm_free(rigold);
  libxsmm_free(rfgold);
  libxsmm_free(rogold);
  libxsmm_free(rcgold);
  libxsmm_free(bigold);
  libxsmm_free(bfgold);
  libxsmm_free(bogold);
  libxsmm_free(bcgold);
  libxsmm_free(csgoldt);
  libxsmm_free(hgoldt);
  libxsmm_free(dxgoldt);
  libxsmm_free(dcspgold);
  libxsmm_free(dhpgold);
  libxsmm_free(dwgold);
#if defined(TWO_GEMMS)
  libxsmm_free(drgold);
#endif
  libxsmm_free(dbgold);
  libxsmm_free(dcsgold);
  libxsmm_free(dhgoldt);
  libxsmm_free(xt);
  libxsmm_free(csp);
  libxsmm_free(hp);
  libxsmm_free(w);
  libxsmm_free(r);
  libxsmm_free(b);
  libxsmm_free(cst);
  libxsmm_free(ht);
  libxsmm_free(dxt);
  libxsmm_free(dcsp);
  libxsmm_free(dhp);
  libxsmm_free(dw);
  libxsmm_free(dr);
  libxsmm_free(db);
  libxsmm_free(dcs);
  libxsmm_free(dht);
  libxsmm_free(it);
  libxsmm_free(ft);
  libxsmm_free(ot);
  libxsmm_free(cit);
  libxsmm_free(cot);

  { const char *const env_check_scale = getenv("CHECK_SCALE");
    const double check_scale = LIBXSMM_ABS(0 == env_check_scale ? 1.0 : atof(env_check_scale));
    if (LIBXSMM_NEQ(0, check) && (check < 100.0 * check_scale * diff.normf_rel) && (global_status == LIBXSMM_DNN_SUCCESS)) {
      fprintf(stderr, "FAILED with an error of %f%%!\n", 100.0 * diff.normf_rel);
      exit(EXIT_FAILURE);
    }
  }

  /* some empty lines at the end */
  printf("\n\n\n");

  return EXIT_SUCCESS;
}

