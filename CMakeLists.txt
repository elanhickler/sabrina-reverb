cmake_minimum_required(VERSION 3.28)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.15 CACHE STRING "Build for 10.14")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# TODO Remove This before we ship non beta version. Remember
# the AU rescans only when versions change even if binaries are
# different. Used as every product's BUNDLE_VERSION; the user-facing
# version comes from each product's own definition.
string(TIMESTAMP DAY_OF_YEAR "%j")
string(TIMESTAMP YEAR "%Y")
math(EXPR PART0 "${YEAR}-2025 + 5")
math(EXPR PART1 "${DAY_OF_YEAR} + 1")

project(soemdsp-sandbox-plugins VERSION 0.${PART0}.${PART1} LANGUAGES C CXX)

option(USE_SANITIZER "Build and link with ASAN" FALSE)
option(COPY_AFTER_BUILD "Will copy after build" TRUE)
set(SE_PRODUCTS "all" CACHE STRING "Products to build: all, or a ;-list of slugs under products/")

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

set(JUCE_PATH "${CMAKE_SOURCE_DIR}/libs/JUCE")
add_subdirectory(libs)

# soemdsp plugin support (Parameter / curve / string helpers)
include(cmake/soemdsp.cmake)

include(cmake/add_product.cmake)

file(GLOB _se_product_dirs LIST_DIRECTORIES true "${CMAKE_SOURCE_DIR}/products/*")
foreach (_dir ${_se_product_dirs})
    if (IS_DIRECTORY "${_dir}" AND EXISTS "${_dir}/product.cmake")
        get_filename_component(_slug "${_dir}" NAME)
        if ("${SE_PRODUCTS}" STREQUAL "all" OR "${_slug}" IN_LIST SE_PRODUCTS)
            add_product(${_slug})
        endif()
    endif()
endforeach()
