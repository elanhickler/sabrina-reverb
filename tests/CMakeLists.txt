add_executable(${PROJECT_NAME}-tests test_main.cpp dsp_basics.cpp golden_render.cpp golden_engine.cpp)
target_compile_definitions(${PROJECT_NAME}-tests PRIVATE
        SE_GOLDEN_DIR="${CMAKE_CURRENT_SOURCE_DIR}/golden")
set_target_properties(${PROJECT_NAME}-tests PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
target_link_libraries(${PROJECT_NAME}-tests
        ${PROJECT_NAME}-impl
        fmt
        sst-plugininfra::version_information
        sst-basic-blocks sst-cpputils sst-plugininfra
        sst-plugininfra::filesystem sst-plugininfra::patchbase sst-plugininfra::tinyxml
        clap-helpers simde fmt-header-only
        soemdsp::plugin_support magic_enum::magic_enum
        catch2)
