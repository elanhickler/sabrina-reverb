cmake_minimum_required(VERSION 3.28)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.15 CACHE STRING "Build for 10.14")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# TODO Remove This before we ship non beta version. Remember
# the AU rescans only when versions change even if binaries are
# different.
string(TIMESTAMP DAY_OF_YEAR "%j")
string(TIMESTAMP YEAR "%Y")
math(EXPR PART0 "${YEAR}-2025 + 5")
math(EXPR PART1 "${DAY_OF_YEAR} + 1")

project(sabrina-reverb-clap VERSION 0.${PART0}.${PART1} LANGUAGES C CXX)
set(PRODUCT_NAME "Sabrina Reverb")


option(USE_SANITIZER "Build and link with ASAN" FALSE)
option(COPY_AFTER_BUILD "Will copy after build" TRUE)
option(BUILD_SINGLE_ONLY "Only build the one plugin - no seven sines out" FALSE)

include(cmake/compile-options.cmake)

# GitHub MSVC rejects `enum Nested::ValueType` in sst-plugininfra userdefaults.h
set(_sst_ud "${CMAKE_SOURCE_DIR}/libs/sst/sst-plugininfra/include/sst/plugininfra/userdefaults.h")
if (EXISTS "${_sst_ud}")
    file(READ "${_sst_ud}" _sst_ud_txt)
    if (_sst_ud_txt MATCHES "enum UserDefaultValue::ValueType")
        string(REPLACE
            "v.type = (enum UserDefaultValue::ValueType)vt;"
            "v.type = static_cast<typename UserDefaultValue::ValueType>(vt);"
            _sst_ud_txt "${_sst_ud_txt}")
        file(WRITE "${_sst_ud}" "${_sst_ud_txt}")
    endif()
endif()

## New version
include(libs/sst/sst-plugininfra/cmake/git-version-functions.cmake)
version_from_versionfile_or_git()
message(STATUS "DISPLAY_VERSION=${GIT_IMPLIED_DISPLAY_VERSION}; COMMIT_HASH=${GIT_COMMIT_HASH}; BRANCH=${GIT_BRANCH}")


include(cmake/CmakeRC.cmake)
file(GLOB_RECURSE PATCHES  "resources/factory_patches/*.sxsnp")
cmrc_add_resource_library(${PROJECT_NAME}-patches NAMESPACE sidequest_patches ${PATCHES})

set(JUCE_PATH "${CMAKE_SOURCE_DIR}/libs/JUCE")
add_subdirectory(libs)

# soemdsp (Parameter / SmootherManager / DirtyUpdater / delay::Reverb)
include(cmake/soemdsp.cmake)

add_library(${PROJECT_NAME}-impl STATIC
        src/clap/plugin-clap.cpp
        src/clap/plugin-clap-entry-impl.cpp

        src/ui/plugin-editor.cpp
        src/ui/main-panel.cpp
        src/ui/sabrina-face.cpp

        src/presets/preset-manager.cpp

        src/engine/engine.cpp
        src/engine/voice.cpp
        src/engine/patch.cpp

)
target_include_directories(${PROJECT_NAME}-impl PUBLIC
        src
        "${SOEMDSP_ROOT}/include"
)
# soemdsp plugin Parameter headers need C++20 (std::source_location).
set_target_properties(${PROJECT_NAME}-impl PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
)
# Important setup variables
target_compile_definitions(${PROJECT_NAME}-impl PRIVATE
        PRODUCT_NAME="${PRODUCT_NAME}"
        PATCH_EXTENSION=".sabrina"
)

if (${BUILD_SINGLE_ONLY})
    message(STATUS "Building single plugin version only")
    target_compile_definitions(${PROJECT_NAME}-impl PRIVATE
            CLAP_PLUGIN_COUNT=1
    )
endif()

if (WIN32)
    message(STATUS "Activating wchar presets")
    target_compile_definitions(${PROJECT_NAME}-impl PUBLIC USE_WCHAR_PRESET=1 NOMINMAX)
endif()

target_link_libraries(${PROJECT_NAME}-impl PUBLIC
        clap
)
target_link_libraries(${PROJECT_NAME}-impl PRIVATE
        simde
        clap-helpers clap-wrapper-extensions
        mts-esp-client
        fmt-header-only
        sst-basic-blocks sst-voicemanager sst-jucegui sst-cpputils
        sst-plugininfra
        sst-plugininfra::filesystem
        sst-plugininfra::tinyxml
        sst-plugininfra::strnatcmp
        sst-plugininfra::patchbase
        sst-plugininfra::version_information
        sst::clap_juce_shim sst::clap_juce_shim_headers
        soemdsp::sabrina
        soemdsp::plugin_support
        magic_enum::magic_enum
        ${PROJECT_NAME}-patches
)

make_clapfirst_plugins(
        TARGET_NAME ${PROJECT_NAME}
        IMPL_TARGET ${PROJECT_NAME}-impl

        OUTPUT_NAME "${PRODUCT_NAME}"

        ENTRY_SOURCE src/clap/plugin-clap-entry.cpp

        BUNDLE_IDENTIFER "org.soundemote.sabrina-reverb"
        BUNDLE_VERSION ${PROJECT_VERSION}

        COPY_AFTER_BUILD ${COPY_AFTER_BUILD}

        PLUGIN_FORMATS CLAP VST3 AUV2

        ASSET_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${PROJECT_NAME}_assets

        # You can add a target-per-standalone you want. Syntax here is
        #   target-postfix output-name clap-id
        # This allows you to make multiple standalones from a multi-plugin clap
        STANDALONE_CONFIGURATIONS
            standalone "${PRODUCT_NAME}" "org.soundemote.sabrina-reverb"

        # STANDALONE_MACOS_ICON "${CMAKE_SOURCE_DIR}/resources/mac_installer/Icon.icns"
)

add_subdirectory(tests)
