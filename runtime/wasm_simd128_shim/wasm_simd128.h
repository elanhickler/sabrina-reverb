// SSE2 stand-in for the wasm_simd128.h calls Sabrina Reverb uses.
// Only the plugin native build sees this header.
#pragma once

#include <cmath>
#include <emmintrin.h>

using v128_t = __m128d;

inline v128_t wasm_f64x2_splat(double x) { return _mm_set1_pd(x); }

inline v128_t wasm_f64x2_make(double lane0, double lane1) {
    return _mm_set_pd(lane1, lane0);
}

inline v128_t wasm_f64x2_add(v128_t a, v128_t b) { return _mm_add_pd(a, b); }
inline v128_t wasm_f64x2_sub(v128_t a, v128_t b) { return _mm_sub_pd(a, b); }
inline v128_t wasm_f64x2_mul(v128_t a, v128_t b) { return _mm_mul_pd(a, b); }
inline v128_t wasm_f64x2_div(v128_t a, v128_t b) { return _mm_div_pd(a, b); }

inline v128_t wasm_f64x2_abs(v128_t a) {
    const __m128d sign = _mm_set1_pd(-0.0);
    return _mm_andnot_pd(sign, a);
}

inline v128_t wasm_f64x2_floor(v128_t a) {
    alignas(16) double x[2];
    _mm_store_pd(x, a);
    x[0] = std::floor(x[0]);
    x[1] = std::floor(x[1]);
    return _mm_load_pd(x);
}

inline v128_t wasm_f64x2_pmin(v128_t a, v128_t b) { return _mm_min_pd(a, b); }
inline v128_t wasm_f64x2_pmax(v128_t a, v128_t b) { return _mm_max_pd(a, b); }

inline void wasm_v128_store(void* p, v128_t v) {
    _mm_storeu_pd(static_cast<double*>(p), v);
}
