cmake_minimum_required(VERSION 3.22)

project(Voxline VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# The bundled toolchain lives beside the sibling SightLine checkout.
set(VX_JUCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../SightLine/.tools/JUCE-8.0.6"
    CACHE PATH "Path to a JUCE 8 checkout")

set(VX_CORE_SOURCES
    Source/core/Modules.cpp
    Source/core/Reverb.cpp
    Source/core/Tune.cpp
    Source/core/Chain.cpp)

# ---------------------------------------------------------------------------
# Headless DSP tests. No JUCE dependency; this is what build.sh tests runs.
# ---------------------------------------------------------------------------
add_executable(VoxlineTests Tests/main.cpp ${VX_CORE_SOURCES})
target_include_directories(VoxlineTests PRIVATE Source third_party)

if (NOT MSVC)
    target_compile_options(VoxlineTests PRIVATE -Wall -Wextra -Wno-unused-parameter)
    target_link_options(VoxlineTests PRIVATE -static -static-libgcc -static-libstdc++)
endif()

# ---------------------------------------------------------------------------
# The plugin.
# ---------------------------------------------------------------------------
if (EXISTS "${VX_JUCE_DIR}/CMakeLists.txt")
    add_subdirectory("${VX_JUCE_DIR}" "${CMAKE_BINARY_DIR}/juce" EXCLUDE_FROM_ALL)

    juce_add_plugin(Voxline
        COMPANY_NAME              "Cdbx"
        BUNDLE_ID                 "com.codebuff.voxline"
        PLUGIN_MANUFACTURER_CODE  Cdbx
        PLUGIN_CODE               Vxln
        PRODUCT_NAME              "voxline"
        DESCRIPTION               "Tune, shape and space for a vocal in one fast chain"
        IS_SYNTH                  FALSE
        NEEDS_MIDI_INPUT          FALSE
        NEEDS_MIDI_OUTPUT         FALSE
        IS_MIDI_EFFECT            FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS FALSE
        COPY_PLUGIN_AFTER_BUILD   FALSE
        FORMATS                   VST3 Standalone
        VST3_CATEGORIES           "Fx")

    target_sources(Voxline PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/Parameters.cpp
        Source/Presets.cpp
        Source/ui/ChainView.cpp
        Source/ui/Graphs.cpp
        ${VX_CORE_SOURCES})

    target_include_directories(Voxline PRIVATE Source third_party)

    target_compile_definitions(Voxline PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_REPORT_APP_USAGE=0
        JUCE_STRICT_REFCOUNTEDPOINTER=1)

    set(VX_WIN_LIBS
        wininet winmm ws2_32 ole32 oleaut32 uuid version shlwapi user32 gdi32
        shell32 comdlg32 imm32 rstrtmgr dwmapi d2d1 d3d11 dxgi dcomp
        windowscodecs setupapi crypt32 secur32 winhttp uxtheme oleacc propsys
        hid shcore dbghelp comctl32 avrt ksuser)

    target_link_libraries(Voxline PRIVATE
        juce::juce_audio_utils
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
        ${VX_WIN_LIBS})

    if (WIN32)
        target_sources(Voxline_Standalone PRIVATE Source/platform/win_manifest.rc)
        set_source_files_properties(Source/platform/win_manifest.rc PROPERTIES
            COMPILE_FLAGS "--include-dir=${CMAKE_CURRENT_SOURCE_DIR}/Source/platform")

        target_sources(Voxline_VST3 PRIVATE Source/platform/dll_manifest.rc)
        set_source_files_properties(Source/platform/dll_manifest.rc PROPERTIES
            COMPILE_FLAGS "--include-dir=${CMAKE_CURRENT_SOURCE_DIR}/Source/platform")
    endif()

    if (NOT MSVC)
        foreach (vxTarget Voxline Voxline_VST3 Voxline_Standalone)
            if (TARGET ${vxTarget})
                target_link_options(${vxTarget} PRIVATE -static -static-libgcc -static-libstdc++)
            endif()
        endforeach()
    endif()

    # -----------------------------------------------------------------------
    # Console smoke harness: renders every preset, saves/loads state, and
    # writes editor snapshots to PNG for a visual check.
    # -----------------------------------------------------------------------
    add_executable(VoxlineSmoke
        Tests/smoke.cpp
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/Parameters.cpp
        Source/Presets.cpp
        Source/ui/ChainView.cpp
        Source/ui/Graphs.cpp
        ${VX_CORE_SOURCES})

    target_include_directories(VoxlineSmoke PRIVATE Source third_party)

    if (WIN32)
        target_sources(VoxlineSmoke PRIVATE Source/platform/win_manifest.rc)
        set_source_files_properties(Source/platform/win_manifest.rc PROPERTIES
            COMPILE_FLAGS "--include-dir=${CMAKE_CURRENT_SOURCE_DIR}/Source/platform")
    endif()

    target_compile_definitions(VoxlineSmoke PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_MODAL_LOOPS_PERMITTED=1
        JUCE_STRICT_REFCOUNTEDPOINTER=1)

    target_link_libraries(VoxlineSmoke PRIVATE
        juce::juce_audio_utils
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
        ${VX_WIN_LIBS})

    if (NOT MSVC)
        target_link_options(VoxlineSmoke PRIVATE -static -static-libgcc -static-libstdc++)
    endif()

    set_target_properties(VoxlineSmoke PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
else()
    message(WARNING "JUCE not found at ${VX_JUCE_DIR}; only the DSP tests will build.")
endif()
