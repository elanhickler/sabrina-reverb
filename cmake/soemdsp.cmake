# soemdsp plugin support (Parameter / curve / string helpers), shared by all
# products. Product DSP kernels are built per product in add_product.cmake.

set(SOEMDSP_ROOT "${CMAKE_SOURCE_DIR}/libs/soemdsp" CACHE PATH "Path to soemdsp")
if (NOT EXISTS "${SOEMDSP_ROOT}/include/soemdsp/plugin/Parameter.hpp")
    message(FATAL_ERROR
        "soemdsp plugin headers not found at ${SOEMDSP_ROOT}. "
        "This repo vendors soemdsp as a git submodule: "
        "git submodule update --init --recursive")
endif()

include(FetchContent)
FetchContent_Declare(magic_enum
    GIT_REPOSITORY https://github.com/Neargye/magic_enum.git
    GIT_TAG v0.9.7
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(magic_enum)

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
