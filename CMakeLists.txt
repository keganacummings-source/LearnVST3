# ============================================================================
# BBGVST - BUILD FILE
# ============================================================================
# This file tells CMake how to build the plugin.
#
# If you are a beginner, think of CMake as a recipe:
#   1. Find/download JUCE.
#   2. Create a plugin named BBGVST.
#   3. Give that plugin our .cpp/.h files.
#   4. Link the JUCE modules we use.
#   5. Turn the source into a VST3 (and a Standalone app).
#
# The important rule for studying this file is: the C++ lives in Source/.
# This file only describes how the compiler should assemble it.
# ============================================================================

cmake_minimum_required(VERSION 3.22)
project(BBGVST VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Universal macOS build" FORCE)
endif()

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 7.0.12
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(BBGVST
    COMPANY_NAME "KeganaCummings"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    PLUGIN_MANUFACTURER_CODE Kcum
    PLUGIN_CODE BBFX
    FORMATS VST3 Standalone
    VST3_CATEGORIES "Fx" "Instrument"
    PRODUCT_NAME "BBGVST FX + Instruments"
    DESCRIPTION "Massive original FX, synth, drum and instrument workstation"
)

target_sources(BBGVST PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/FxRackDsp.h
    Source/FxCatalog.h
    Source/InstrumentCatalog.h
    Source/CustomLookAndFeel.h
)

target_link_libraries(BBGVST PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_dsp
    juce::juce_gui_basics
    juce::juce_data_structures
    juce::juce_events
    juce::juce_core
)

if(MSVC)
    target_compile_options(BBGVST PRIVATE /utf-8 /MP)
endif()

target_compile_definitions(BBGVST PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    BBGVST_VERSION="1.0.0"
)

set_target_properties(BBGVST PROPERTIES
    JUCE_COPY_PLUGIN_AFTER_BUILD TRUE
)
