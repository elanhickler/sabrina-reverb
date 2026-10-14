# add_product(<slug>)
#
# Builds one product from the shared shell/ code plus products/<slug>/.
# Identity, params and layout come from products/<slug>/product.json (see
# cmake/product_json.cmake, which also checks params.lock and generates
# product_generated.h). products/<slug>/product.cmake only lists sources:
#   SE_PRODUCT_SOURCES       product C++ (kernel adapter, face), relative to products/<slug>/
#   SE_PRODUCT_DSP_SOURCES   kernel C++ (vendored sandbox native module), relative
#   SE_PRODUCT_TESTS         product test sources, relative (optional)
#
# Targets: <slug>-dsp, <slug>-impl, <slug>_clap / _vst3 / _auv2 / _standalone,
#          <slug>_all, <slug>-tests. Assets land in build/<slug>_assets.

include(${CMAKE_CURRENT_LIST_DIR}/product_json.cmake)

set(SE_SHELL_SOURCES
        shell/clap/plugin-clap.cpp
        shell/clap/plugin-clap-entry-impl.cpp
        shell/ui/plugin-editor.cpp
        shell/ui/main-panel.cpp
        shell/presets/preset-manager.cpp
        shell/engine/engine.cpp
        shell/engine/patch.cpp
)

function(add_product slug)
    set(_pdir "${CMAKE_SOURCE_DIR}/products/${slug}")
    unset(SE_PRODUCT_TESTS)
    unset(SE_PRODUCT_SOURCES)
    include("${_pdir}/product.cmake")
    se_load_product(${slug})
    foreach (_v SE_PRODUCT_DSP_SOURCES)
        if (NOT DEFINED ${_v})
            message(FATAL_ERROR "products/${slug}/product.cmake must set ${_v}")
        endif()
    endforeach()
    message(STATUS "Product ${slug}: ${SE_PRODUCT_NAME} ${SE_PRODUCT_VERSION} (${SE_PRODUCT_CLAP_ID})")

    # --- DSP kernel (native C++ only, APP_POLICY) ---
    list(TRANSFORM SE_PRODUCT_DSP_SOURCES PREPEND "${_pdir}/")
    add_library(${slug}-dsp STATIC ${SE_PRODUCT_DSP_SOURCES})
    target_include_directories(${slug}-dsp PUBLIC
            "${CMAKE_SOURCE_DIR}/runtime"
            "${CMAKE_SOURCE_DIR}/runtime/wasm_simd128_shim"
            "${CMAKE_SOURCE_DIR}/dsp-common/include"
    )
    target_compile_definitions(${slug}-dsp PUBLIC NOMINMAX)
    target_compile_features(${slug}-dsp PUBLIC cxx_std_17)
    if (MSVC)
        target_compile_options(${slug}-dsp PRIVATE
                /FI"${CMAKE_SOURCE_DIR}/runtime/msvc_builtins.h"
                /wd4244 /wd4267 /wd4305
        )
    endif()

    # --- factory presets ---
    file(GLOB_RECURSE _patches "${_pdir}/presets/*${SE_PRODUCT_PATCH_EXT}" "${_pdir}/presets/*.sxsnp")
    cmrc_add_resource_library(${slug}-patches NAMESPACE se_presets ${_patches})

    # --- shell + product glue ---
    set(_srcs ${SE_SHELL_SOURCES})
    list(TRANSFORM _srcs PREPEND "${CMAKE_SOURCE_DIR}/")
    set(_psrcs ${SE_PRODUCT_SOURCES})
    list(TRANSFORM _psrcs PREPEND "${_pdir}/")
    add_library(${slug}-impl STATIC ${_srcs} ${_psrcs})
    target_include_directories(${slug}-impl PUBLIC
            "${CMAKE_SOURCE_DIR}/shell"
            "${_pdir}/src"
            "${SE_PRODUCT_GENERATED_DIR}"
            "${SOEMDSP_ROOT}/include"
    )
    # soemdsp plugin Parameter headers need C++20 (std::source_location).
    set_target_properties(${slug}-impl PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
    target_compile_definitions(${slug}-impl PRIVATE
            PRODUCT_NAME="${SE_PRODUCT_NAME}"
            PRODUCT_VERSION="${SE_PRODUCT_VERSION}"
            PATCH_EXTENSION="${SE_PRODUCT_PATCH_EXT}"
    )
    if (WIN32)
        target_compile_definitions(${slug}-impl PUBLIC USE_WCHAR_PRESET=1 NOMINMAX)
    endif()
    target_link_libraries(${slug}-impl PUBLIC clap)
    target_link_libraries(${slug}-impl PRIVATE
            simde
            clap-helpers clap-wrapper-extensions
            fmt-header-only
            sst-basic-blocks sst-jucegui sst-cpputils
            sst-plugininfra
            sst-plugininfra::filesystem
            sst-plugininfra::tinyxml
            sst-plugininfra::strnatcmp
            sst-plugininfra::patchbase
            sst-plugininfra::version_information
            sst::clap_juce_shim sst::clap_juce_shim_headers
            juce::juce_opengl
            ${slug}-dsp
            soemdsp::plugin_support
            magic_enum::magic_enum
            ${slug}-patches
    )

    separate_arguments(_formats UNIX_COMMAND "${SE_PRODUCT_FORMATS}")
    make_clapfirst_plugins(
            TARGET_NAME ${slug}
            IMPL_TARGET ${slug}-impl
            OUTPUT_NAME "${SE_PRODUCT_NAME}"
            ENTRY_SOURCE shell/clap/plugin-clap-entry.cpp
            BUNDLE_IDENTIFER "${SE_PRODUCT_CLAP_ID}"
            BUNDLE_VERSION ${PROJECT_VERSION}
            COPY_AFTER_BUILD ${COPY_AFTER_BUILD}
            PLUGIN_FORMATS ${_formats}
            ASSET_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${slug}_assets
            STANDALONE_CONFIGURATIONS
                standalone "${SE_PRODUCT_NAME}" "${SE_PRODUCT_CLAP_ID}"
    )

    # --- tests: shared + product (golden render etc.) ---
    set(_tsrcs ${SE_PRODUCT_TESTS})
    list(TRANSFORM _tsrcs PREPEND "${_pdir}/")
    add_executable(${slug}-tests
            "${CMAKE_SOURCE_DIR}/tests/test_main.cpp"
            "${CMAKE_SOURCE_DIR}/tests/dsp_basics.cpp"
            ${_tsrcs})
    target_compile_definitions(${slug}-tests PRIVATE SE_GOLDEN_DIR="${_pdir}/tests/golden")
    set_target_properties(${slug}-tests PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
    target_link_libraries(${slug}-tests
            ${slug}-impl
            fmt
            sst-plugininfra::version_information
            sst-basic-blocks sst-cpputils sst-plugininfra
            sst-plugininfra::filesystem sst-plugininfra::patchbase sst-plugininfra::tinyxml
            clap-helpers simde fmt-header-only
            soemdsp::plugin_support magic_enum::magic_enum
            catch2)
endfunction()
