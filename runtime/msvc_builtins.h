#pragma once
#include <cmath>
#if defined(_MSC_VER) && !defined(__clang__)
#define __builtin_sqrt(x) std::sqrt(x)
#define __builtin_floor(x) std::floor(x)
#define __builtin_fabs(x) std::fabs(x)
#endif
