#ifndef SIMD_H
#define SIMD_H

static_assert(sizeof(float) == 4);

// If any SIMD are available (SSE2, AVX, AVX-512), define "HAVE_SIMD". If none are
// available, then don't define it, as its absence means to use non-vectorized paths.

// @todo: need to figure out how to define SIMD #defines on Windows.
#if defined(__AVX512F__)
#define HAVE_SIMD
#define HAVE_SIMD_AVX512
#elif defined(__AVX__)
#define HAVE_SIMD
#define HAVE_SIMD_AVX
#elif defined(__SSE2__)
#define HAVE_SIMD
#define HAVE_SIMD_SSE2
#endif

#if defined(HAVE_SIMD_AVX512)
#include <immintrin.h>
#define simd_type __m512
#define simd_type_i __m512i
#define simd_align alignof(simd_type)
#define simd_size (sizeof(simd_type) / sizeof(float))
static_assert(simd_size == 16);
#define simd_tile_width 4
#define simd_tile_height 4

#define simd_setzero() _mm512_setzero_ps()
#define simd_set1(a) _mm512_set1_ps(a)
#define simd_load(ptr) _mm512_load_ps(ptr)
#define simd_store(ptr, a) _mm512_store_ps(ptr, a)
#define simd_add(a, b) _mm512_add_ps(a, b)
#define simd_sub(a, b) _mm512_sub_ps(a, b)
#define simd_mul(a, b) _mm512_mul_ps(a, b)
#define simd_div(a, b) _mm512_div_ps(a, b)
#define simd_min(a, b) _mm512_min_ps(a, b)
#define simd_max(a, b) _mm512_max_ps(a, b)
#define simd_cvtepi32(a) _mm512_cvtepi32_ps(a)
#elif defined(HAVE_SIMD_AVX)
#include <immintrin.h>
#define simd_type __m256
#define simd_type_i __m256i
#define simd_align alignof(simd_type)
#define simd_size (sizeof(simd_type) / sizeof(float))
static_assert(simd_size == 8);
#define simd_tile_width 4
#define simd_tile_height 2

#define simd_setzero() _mm256_setzero_ps()
#define simd_set1(a) _mm256_set1_ps(a)
#define simd_load(ptr) _mm256_load_ps(ptr)
#define simd_store(ptr, a) _mm256_store_ps(ptr, a)
#define simd_add(a, b) _mm256_add_ps(a, b)
#define simd_sub(a, b) _mm256_sub_ps(a, b)
#define simd_mul(a, b) _mm256_mul_ps(a, b)
#define simd_div(a, b) _mm256_div_ps(a, b)
#define simd_min(a, b) _mm256_min_ps(a, b)
#define simd_max(a, b) _mm256_max_ps(a, b)
#define simd_cvtepi32(a) _mm256_cvtepi32_ps(a)
#elif defined(HAVE_SIMD_SSE2)
#include <xmmintrin.h>
#define simd_type __m128
#define simd_type_i __m128i
#define simd_align alignof(simd_type)
#define simd_size (sizeof(simd_type) / sizeof(float))
static_assert(simd_size == 4);
#define simd_tile_width 2
#define simd_tile_height 2

#define simd_setzero() _mm_setzero_ps()
#define simd_set1(a) _mm_set1_ps(a)
#define simd_load(ptr) _mm_load_ps(ptr)
#define simd_store(ptr, a) _mm_store_ps(ptr, a)
#define simd_add(a, b) _mm_add_ps(a, b)
#define simd_sub(a, b) _mm_sub_ps(a, b)
#define simd_mul(a, b) _mm_mul_ps(a, b)
#define simd_div(a, b) _mm_div_ps(a, b)
#define simd_min(a, b) _mm_min_ps(a, b)
#define simd_max(a, b) _mm_max_ps(a, b)
#define simd_cvtepi32(a) _mm_cvtepi32_ps(a)
#else
// Make sure HAVE_SIMD is not defined so we can still use non-vectorized paths.
#if defined(HAVE_SIMD)
#error No SIMD functions implemented.
#endif
#endif

#endif
