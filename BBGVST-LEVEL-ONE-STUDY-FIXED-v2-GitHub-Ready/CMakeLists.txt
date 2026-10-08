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
    # Build against an older macOS deployment target so JUCE 7 can still
    # compile with modern Xcode/macOS SDKs. This is NOT the minimum machine
    # we are testing on; it is simply the compatibility target for the plugin.
    # JUCE 7.0.12 uses a few Apple APIs that became unavailable when the
    # deployment target is treated as macOS 15 or newer.
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version (universal arm64+x86_64)" FORCE)
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

# ---------------------------------------------------------------------------
# macOS + JUCE 7 compatibility patch
# ---------------------------------------------------------------------------
# JUCE 7.0.12 predates the macOS 15 SDK change that made
# CGWindowListCreateImage unavailable. GitHub's current macOS runner can use
# a newer SDK, so the JUCE helper would fail before our own code even builds.
# Our plugin does not depend on JUCE's native-window screenshot helper, so on
# macOS 15+ we safely make that helper return an empty image instead.
#
# This edits only the downloaded JUCE copy inside the build directory. It does
# NOT modify the user's source files or fork JUCE.
if(APPLE)
    set(BBGVST_JUCE_MAC_FILE "${juce_SOURCE_DIR}/modules/juce_gui_basics/native/juce_Windowing_mac.mm")
    if(EXISTS "${BBGVST_JUCE_MAC_FILE}")
        file(READ "${BBGVST_JUCE_MAC_FILE}" BBGVST_JUCE_MAC_SOURCE)
        set(BBGVST_JUCE_OLD_SNAPSHOT_BLOCK [=[
        // CGWindowListCreateImage is replaced by functions in the ScreenCaptureKit framework, but
        // that framework is only available from macOS 12.3 onwards.
        // A suitable @available check should be added once the minimum build OS is 12.3 or greater,
        // so that ScreenCaptureKit can be weak-linked.
       #if defined (MAC_OS_VERSION_14_0) && MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_VERSION_14_0
        JUCE_BEGIN_IGNORE_WARNINGS_GCC_LIKE ("-Wdeprecated-declarations")
        #define JUCE_DEPRECATION_IGNORED 1
       #endif
        CGImageRef screenShot = CGWindowListCreateImage (CGRectNull,
                                                         kCGWindowListOptionIncludingWindow,
                                                         (CGWindowID) [nsWindow windowNumber],
                                                         kCGWindowImageBoundsIgnoreFraming);

        #if JUCE_DEPRECATION_IGNORED
        JUCE_END_IGNORE_WARNINGS_GCC_LIKE
        #undef JUCE_DEPRECATION_IGNORED
        #endif
]=])
        set(BBGVST_JUCE_NEW_SNAPSHOT_BLOCK [=[
        // JUCE 7.0.12 uses CGWindowListCreateImage here. That API became
        // unavailable in the macOS 15 SDK. This helper is only used for
        // native-window snapshots, which BBGVST does not require.
       #if MAC_OS_X_VERSION_MAX_ALLOWED >= 150000
        juce::ignoreUnused (nsWindow);
        return {};
       #else
        CGImageRef screenShot = CGWindowListCreateImage (CGRectNull,
                                                         kCGWindowListOptionIncludingWindow,
                                                         (CGWindowID) [nsWindow windowNumber],
                                                         kCGWindowImageBoundsIgnoreFraming);
       #endif
]=])
        string(FIND "${BBGVST_JUCE_MAC_SOURCE}" "CGImageRef screenShot = CGWindowListCreateImage" BBGVST_SNAPSHOT_POS)
        if(BBGVST_SNAPSHOT_POS GREATER -1)
            string(REPLACE "${BBGVST_JUCE_OLD_SNAPSHOT_BLOCK}" "${BBGVST_JUCE_NEW_SNAPSHOT_BLOCK}" BBGVST_JUCE_MAC_SOURCE "${BBGVST_JUCE_MAC_SOURCE}")
            file(WRITE "${BBGVST_JUCE_MAC_FILE}" "${BBGVST_JUCE_MAC_SOURCE}")
        endif()
    endif()
endif()

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
