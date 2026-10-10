# Compile the sandbox Sabrina Reverb native module (same DSP as soemdsp-sandbox
# reverbEffect / sabrina_reverb.wasm). Original VST reference lives beside it at
# native_modules/sabrina_reverb/originalcode/.

set(SOEMDSP_SANDBOX "${CMAKE_SOURCE_DIR}/../soemdsp-sandbox" CACHE PATH
    "Path to soundemote/soemdsp-sandbox")

set(SABRINA_SRC "${SOEMDSP_SANDBOX}/native_modules/sabrina_reverb/sabrina_reverb.cpp")
if (NOT EXISTS "${SABRINA_SRC}")
    message(FATAL_ERROR
        "sabrina_reverb.cpp not found at ${SABRINA_SRC}. "
        "Clone https://github.com/soundemote/soemdsp-sandbox next to this repo, "
        "or pass -DSOEMDSP_SANDBOX=...")
endif()

add_library(soemdsp-sabrina STATIC "${SABRINA_SRC}")
add_library(soemdsp::sabrina ALIAS soemdsp-sabrina)

set(SOEMDSP_ROOT "${CMAKE_SOURCE_DIR}/../soemdsp" CACHE PATH "Path to soundemote/soemdsp")
if (NOT EXISTS "${SOEMDSP_ROOT}/include/soemdsp/plugin/Parameter.hpp")
    message(FATAL_ERROR
        "soemdsp plugin headers not found at ${SOEMDSP_ROOT}. "
        "Clone https://github.com/soundemote/soemdsp next to this repo, "
        "or pass -DSOEMDSP_ROOT=...")
endif()

include(FetchContent)
FetchContent_Declare(magic_enum
    GIT_REPOSITORY https://github.com/Neargye/magic_enum.git
    GIT_TAG v0.9.7
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(magic_enum)

# soemdsp plugin Parameter / curve / string helpers (not the sandbox math lib).
add_library(soemdsp-plugin-support STATIC
    "${SOEMDSP_ROOT}/include/soemdsp/utility/src/curve_functions.cpp"
    "${SOEMDSP_ROOT}/include/soemdsp/utility/src/operations_string.cpp"
    "${SOEMDSP_ROOT}/include/soemdsp/utility/src/operations_char.cpp"
    "${SOEMDSP_ROOT}/include/soemdsp/utility/src/StringIterator.cpp"
)
add_library(soemdsp::plugin_support ALIAS soemdsp-plugin-support)
target_include_directories(soemdsp-plugin-support PUBLIC
    "${SOEMDSP_ROOT}/include"
)
target_compile_definitions(soemdsp-plugin-support PUBLIC NOMINMAX)
target_compile_features(soemdsp-plugin-support PUBLIC cxx_std_20)
target_link_libraries(soemdsp-plugin-support PUBLIC
    magic_enum::magic_enum
    fmt-header-only
)

target_include_directories(soemdsp-sabrina PUBLIC
    "${CMAKE_SOURCE_DIR}/runtime"
    "${CMAKE_SOURCE_DIR}/runtime/wasm_simd128_shim"
    "${SOEMDSP_SANDBOX}/library/include"
)

target_compile_definitions(soemdsp-sabrina PUBLIC NOMINMAX)
target_compile_features(soemdsp-sabrina PUBLIC cxx_std_17)

if (MSVC)
    target_compile_options(soemdsp-sabrina PRIVATE
        /FI"${CMAKE_SOURCE_DIR}/runtime/msvc_builtins.h"
        /wd4244 /wd4267 /wd4305
    )
endif()
