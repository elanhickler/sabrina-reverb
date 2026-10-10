// Native stand-in for wasm_simd128.h used by sabrina_reverb.cpp.
// x86 uses SSE2. Everything else (macOS ARM) uses two doubles.
#pragma once

#include <cmath>

#if defined(_M_X64) || defined(__x86_64__) || defined(_M_IX86) || defined(__i386__)

#include <emmintrin.h>

using v128_t = __m128d;

inline v128_t wasm_f64x2_splat(double x) { return _mm_set1_pd(x); }
inline v128_t wasm_f64x2_make(double lane0, double lane1) { return _mm_set_pd(lane1, lane0); }
inline v128_t wasm_f64x2_add(v128_t a, v128_t b) { return _mm_add_pd(a, b); }
inline v128_t wasm_f64x2_sub(v128_t a, v128_t b) { return _mm_sub_pd(a, b); }
inline v128_t wasm_f64x2_mul(v128_t a, v128_t b) { return _mm_mul_pd(a, b); }
inline v128_t wasm_f64x2_div(v128_t a, v128_t b) { return _mm_div_pd(a, b); }
inline v128_t wasm_f64x2_abs(v128_t a)
{
    const __m128d sign = _mm_set1_pd(-0.0);
    return _mm_andnot_pd(sign, a);
}
inline v128_t wasm_f64x2_floor(v128_t a)
{
    alignas(16) double x[2];
    _mm_store_pd(x, a);
    x[0] = std::floor(x[0]);
    x[1] = std::floor(x[1]);
    return _mm_load_pd(x);
}
inline v128_t wasm_f64x2_pmin(v128_t a, v128_t b) { return _mm_min_pd(a, b); }
inline v128_t wasm_f64x2_pmax(v128_t a, v128_t b) { return _mm_max_pd(a, b); }
inline void wasm_v128_store(void *p, v128_t v)
{
    _mm_storeu_pd(static_cast<double *>(p), v);
}

#else

struct v128_t
{
    double lane0;
    double lane1;
};

inline v128_t wasm_f64x2_splat(double x) { return {x, x}; }
inline v128_t wasm_f64x2_make(double lane0, double lane1) { return {lane0, lane1}; }
inline v128_t wasm_f64x2_add(v128_t a, v128_t b) { return {a.lane0 + b.lane0, a.lane1 + b.lane1}; }
inline v128_t wasm_f64x2_sub(v128_t a, v128_t b) { return {a.lane0 - b.lane0, a.lane1 - b.lane1}; }
inline v128_t wasm_f64x2_mul(v128_t a, v128_t b) { return {a.lane0 * b.lane0, a.lane1 * b.lane1}; }
inline v128_t wasm_f64x2_div(v128_t a, v128_t b) { return {a.lane0 / b.lane0, a.lane1 / b.lane1}; }
inline v128_t wasm_f64x2_abs(v128_t a) { return {std::fabs(a.lane0), std::fabs(a.lane1)}; }
inline v128_t wasm_f64x2_floor(v128_t a) { return {std::floor(a.lane0), std::floor(a.lane1)}; }
inline v128_t wasm_f64x2_pmin(v128_t a, v128_t b)
{
    return {a.lane0 < b.lane0 ? a.lane0 : b.lane0, a.lane1 < b.lane1 ? a.lane1 : b.lane1};
}
inline v128_t wasm_f64x2_pmax(v128_t a, v128_t b)
{
    return {a.lane0 > b.lane0 ? a.lane0 : b.lane0, a.lane1 > b.lane1 ? a.lane1 : b.lane1};
}
inline void wasm_v128_store(void *p, v128_t v)
{
    auto *out = static_cast<double *>(p);
    out[0] = v.lane0;
    out[1] = v.lane1;
}

#endif
